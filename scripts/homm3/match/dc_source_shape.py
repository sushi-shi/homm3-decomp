#!/usr/bin/env python3
"""Fatal gate for Dreamcast-attested helper shape in reconstructed functions.

Retail bytes remain authoritative, but an exact x86 lowering is not sufficient
when the Dreamcast CodeView call graph proves that the original source used a
named helper.  For every function carrying a ``dc 0x...`` source claim, and
every active unclaimed definition retaining its ``E:\\gamedcs`` provenance
line, this gate checks that each source-visible game helper from
``evidence/dc-xref-graph.tsv`` is still named in the reconstructed C++ body.
The only exceptions are proof-carrying Complete changes: a transfer requires
both the old caller's forwarding shape and an exact retail receiver, a bounded
call-spelling substitution requires its caller itself to remain exact, and a
DC-only helper order requires an exact retail caller plus the independently
measured Complete source shape.  These are stricter than waivers and record
independently proved source changes.
Provenance-marked header definitions are audited by original source file and
line rather than by emitting object, because `/Ob2` may inline every retail
copy and because one header body can be shared by several TUs.

There is no score threshold or score-based waiver: a local byte maximum cannot
override an attested source fact.  The unfinished reconstruction backlog is
frozen per Dreamcast caller/callee or source-contract identity.  A new omission
is fatal even when another omission disappeared in the same build.  Every pass
rolls restored rows down-only, even while unrelated new omissions keep the gate
red, so concurrent defects cannot delay banking a restored helper.  The
embedded negative controls cover that ratchet as well as
the SetupHeroView defect that caused this gate to land, reordered helpers
inside one CodeView group, and flattening two distinct breakpoint groups
together.
"""
from __future__ import annotations

import bisect
import csv
import io
import json
import re
import sys
from collections import Counter
from dataclasses import dataclass

from homm3.analysis import dc_asm, dc_lines, dreamcast
from homm3.core import common
from homm3.match import status
from homm3.vc6 import _source


XREFS = common.HOMM3_DIR / "evidence/dc-xref-graph.tsv"
BASELINE = common.HOMM3_DIR / "config/dc-source-shape-baseline.tsv"
BUILD_REPORT_LIMIT = 25
PROVENANCE_RE = re.compile(
    r"(?m)^[ \t]*//[ \t]*([^:\r\n]+:\\[^:\r\n]+):(\d+)[^\r\n]*$")


@dataclass(frozen=True)
class MissingCall:
    va: int | None
    dc_module: str
    dc_offset: int
    source: str
    line: int
    caller: str
    callee: str
    helper: str


@dataclass(frozen=True)
class MissingDefinition:
    dc_module: str
    dc_offset: int
    source: str
    line: int
    caller: str


@dataclass(frozen=True)
class CallGroup:
    line: int
    callees: tuple[str, ...]


@dataclass(frozen=True)
class MisgroupedCalls:
    va: int | None
    dc_module: str
    dc_offset: int
    source: str
    line: int
    caller: str
    group: CallGroup


@dataclass(frozen=True)
class SourceRule:
    description: str
    pattern: str
    minimum: int = 1
    maximum: int | None = None
    include_directives: bool = False


@dataclass(frozen=True)
class CallTransfer:
    description: str
    receiver_path: str
    receiver_name: str
    receiver_va: int
    caller_pattern: str
    receiver_pattern: str


@dataclass(frozen=True)
class CallSpelling:
    description: str
    caller_va: int
    callee: str
    retail_pattern: str
    canonical_name: str


@dataclass(frozen=True)
class ProvenOrderSkew:
    description: str
    caller_va: int
    retail_pattern: str
    dc_only_helpers: tuple[str, ...]


@dataclass(frozen=True)
class ContractViolation:
    va: int | None
    dc_module: str
    dc_offset: int
    source: str
    line: int
    caller: str
    rule: SourceRule


@dataclass(frozen=True)
class FileContractViolation:
    source: str
    line: int
    description: str


@dataclass(frozen=True)
class XrefCall:
    offset: int
    name: str
    pool_refs: int
    bsr_calls: int


@dataclass(frozen=True)
class DecodedShape:
    offsets: frozenset[int]
    groups: tuple[CallGroup, ...]


Violation = MissingCall | MissingDefinition | MisgroupedCalls \
    | ContractViolation | FileContractViolation
AuditScope = tuple[str, str, str]


def _dc_audit_scope(key: tuple[str, int]) -> AuditScope:
    return "dc", key[0], f"0x{key[1]:x}"


def _file_audit_scope(source: str) -> AuditScope:
    return "file", source, "-"


def _key_audit_scope(key: tuple[str, str, str, str]) -> AuditScope:
    if key[0] == "file-contract":
        return _file_audit_scope(key[1])
    return "dc", key[1], key[2]


# Dreamcast sometimes places one operation in two cooperating functions while
# Complete moves the whole operation into one of them.  Admit such a move only
# as a bounded, proof-carrying transfer: the old caller must retain the exact
# forwarding protocol, the receiver source must retain the helper, and that
# receiver must be byte-exact against retail.  A percentage alone can never
# create an entry here.
PROVEN_CALL_TRANSFERS: dict[tuple[str, int, str], CallTransfer] = {
    ("adventureoptionswindow.obj", 0x5204, "game::ShowScenInfo"):
        CallTransfer(
            "Complete moves campaign scenario-info dispatch from the dialog "
            "handler into advManager::DoAdventureOptions",
            "src/advmgr.cpp", "advManager::DoAdventureOptions", 0x0041AB00,
            r"gpWindowManager\s*->\s*dialogReturn\s*=\s*msg\s*->\s*codeY"
            r"\s*;.*?msg\s*->\s*codeY\s*=\s*widget\s*::\s*"
            r"WIDGET_END_DIALOG\s*;.*?msg\s*->\s*codeX\s*=\s*widget\s*::\s*"
            r"WIDGET_END_DIALOG\s*;.*?return\s+MESSAGE_DISPATCH_FORWARD\s*;",
            r"switch\s*\(\s*gpWindowManager\s*->\s*dialogReturn\s*\)"
            r".*?case\s+TAdventureOptionsWindow\s*::\s*VIEW_SCENARIO_ID"
            r"\s*:\s*gpGame\s*->\s*ShowScenInfo\s*\(\s*\)\s*;\s*break\s*;"),
    ("philai.obj", 0x10E3F8, "buy_artifacts"):
        CallTransfer(
            "Complete transfers AI_enter_town's separate artifact-market "
            "purchase into the exact buy_special_building receiver",
            "src/philai.cpp", "buy_special_building", 0x005259E0,
            r"\bbuy_special_building\s*\(\s*current_hero\s*,\s*"
            r"current_town\s*\)\s*;",
            r"\bbuy_artifacts\s*\(\s*current_hero\s*,\s*gpGame\s*->\s*"
            r"field_1f664\s*,\s*market_count\s*\)\s*;"),
}


# Complete sometimes replaces a Dreamcast project helper with an equivalent
# runtime spelling. Admit only the individually proved exact caller, and
# canonicalize the token solely for call-presence/group auditing.
PROVEN_CALL_SPELLINGS: dict[tuple[str, int], tuple[CallSpelling, ...]] = {
    ("game.obj", 0xA99D0): (
        CallSpelling(
            "Complete SaveGame replaces the Dreamcast custom strnicmp with "
            "the CRT _strnicmp import at both reserved-name checks",
            0x004BEEA0, "strnicmp", r"\b_strnicmp(?=\s*\()", "strnicmp"),
    ),
}


# A lexical helper name is normally the most the source-shape pass can prove:
# same-class methods may legitimately be called on a peer object.  Require a
# self receiver only for individually decoded SH4 call sites.  At all four
# IsHost calls below, SetupAdvancedOptions reloads its saved ``this`` into r4
# immediately before the jsr; pDPlay->IsHost() is therefore not that edge.
PROVEN_SELF_CALLS = frozenset({
    ("singleselectionwindow.obj", 0x136388,
     "TSingleSelectionWindow::IsHost"),
})


# A different lexical order is admitted only after the Complete caller was
# exact, the Dreamcast ordering was imposed and measured, and that older shape
# broke the exact lowering.  Keep the named helpers themselves mandatory; only
# their cross-statement order is classified DC-only while the exact caller and
# the bounded Complete source pattern both survive.
PROVEN_ORDER_SKEWS: dict[tuple[str, int], tuple[ProvenOrderSkew, ...]] = {
    ("spells.obj", 0x152DEC): (
        ProvenOrderSkew(
            "find_spell_target's Complete switch is RESURRECTION / "
            "ANIMATE_DEAD / SACRIFICE, not Dreamcast's SACRIFICE / "
            "RESURRECTION / ANIMATE_DEAD order",
            0x005A3950,
            r"case\s+SPELL_RESURRECTION\s*:.*?"
            r"\bfind_resurrection_target\s*\(.*?"
            r"case\s+SPELL_ANIMATE_DEAD\s*:.*?"
            r"\bfind_animate_dead_target\s*\(.*?"
            r"case\s+SPELL_SACRIFICE\s*:.*?"
            r"\bfind_resurrection_target\s*\(.*?\bbreak\s*;.*?"
            r"\}\s*return\s+cells\s*\[\s*hex\s*\]\s*\.\s*"
            r"get_army\s*\(",
            ("combatManager::find_resurrection_target",
             "combatManager::find_animate_dead_target",
             "hexcell::get_army")),
    ),
    ("townmgr.obj", 0x176634): (
        ProvenOrderSkew(
            "DoCommand's Complete hero cases are SWAP / FROM / TO, not "
            "Dreamcast's TO / FROM / SWAP order",
            0x005D4C10,
            r"case\s+TOWN_COMMAND_SWAP_HEROES\s*:.*?"
            r"\bSwapHeroes\s*\(.*?"
            r"case\s+TOWN_COMMAND_MOVE_HERO_FROM_GARRISON\s*:.*?"
            r"\bMoveHeroFromGarrison\s*\(.*?"
            r"case\s+TOWN_COMMAND_MOVE_HERO_TO_GARRISON\s*:.*?"
            r"\bMoveHeroToGarrison\s*\(",
            ("townManager::MoveHeroToGarrison",
             "townManager::MoveHeroFromGarrison",
             "townManager::SwapHeroes")),
    ),
}


# Named calls cover most recoverable shape automatically. These bounded
# contracts preserve source-visible facts that disappear before the SH4 xref
# graph: inlined accessors/operators, a source order hidden by scheduling, and
# nesting within a single attested statement group.
SOURCE_RULES: dict[tuple[str, int], tuple[SourceRule, ...]] = {
    ("advmgr.obj", 0x1A878): (
        SourceRule(
            "SetHeroContext keeps Dreamcast's player, curr and cell local "
            "identities across the local-player and hero lookup statements",
            r"\bplayerData\s*\*\s*player\s*=\s*gpCurrentPlayer\s*;\s*"
            r"if\s*\(\s*waitingPlayer\s*\)\s*player\s*=\s*gpGame\s*->\s*"
            r"GetLocalPlayer\s*\(\s*\)\s*;\s*else\s*inDialog\s*=\s*1\s*;"
            r"\s*player\s*->\s*currHeroId\s*=\s*heroId\s*;\s*"
            r"hero\s*\*\s*curr\s*=\s*&\s*gpGame\s*->\s*heroes\s*"
            r"\[\s*heroId\s*\]\s*;\s*NewmapCell\s*\*\s*cell\s*;"),
        SourceRule(
            "SetHeroContext keeps Dreamcast's found local around FindHero, "
            "the fallback slot and UpdateHeroLocators",
            r"\bint\s+found\s*=\s*player\s*->\s*FindHero\s*\(\s*heroId"
            r"\s*\)\s*;\s*if\s*\(\s*found\s*==\s*-\s*1\s*\)\s*"
            r"found\s*=\s*0\s*;.*?UpdateHeroLocators\s*\(\s*found\s*,"),
        SourceRule(
            "SetHeroContext keeps Dreamcast's get_target helper boundary "
            "inside the guarded routeTarget scope before SeedTo and ShowRoute",
            r"\bseedingValid\s*=\s*0\s*;\s*if\s*\(\s*curr\s*->\s*"
            r"pathTargetX\s*>=\s*0\s*\)\s*\{\s*type_point\s+routeTarget\s*"
            r"=\s*curr\s*->\s*get_target\s*\(\s*\)\s*;\s*SeedTo\s*"
            r"\(\s*routeTarget\s*\)\s*;\s*\}\s*ShowRoute\s*\("),
        SourceRule(
            "SetHeroContext keeps Dreamcast's cell local live through the "
            "tail ground-set test and ambient-music update",
            r"if\s*\(\s*cell\s*->\s*GroundSet\s*!=\s*field_58\s*&&\s*"
            r"draw_changes\s*\)\s*\{\s*field_58\s*=\s*cell\s*->\s*"
            r"GroundSet\s*;\s*gpSoundManager\s*->\s*SwitchAmbientMusic\s*"
            r"\(\s*gTerrainMusicIds\s*\[\s*field_58\s*\]\s*\)\s*;"),
    ),
    ("ai_tactical.obj", 0x40BB8): (
        SourceRule(
            "consider_single_enchantment keeps Dreamcast's sole recorded "
            "value_func local and its member-function invocation boundary",
            r"\A\s*TEnchantValue\s+value_func\s*=\s*"
            r"get_enchantment_function\s*\(\s*choice\s*->\s*spell\s*\)"
            r"\s*;.*?\(\s*this\s*->\s*\*\s*value_func\s*\)\s*\(\s*"
            r"target\s*,\s*\*\s*choice\s*\)"),
    ),
    ("ai_player.obj", 0x34FB8): (
        SourceRule(
            "value_of_hiring keeps Dreamcast's player_id, player, hero_army, "
            "town_army and purchaser root-local setup, including the "
            "non-const town::get_army source call",
            r"\A\s*short\s+player_id\s*=\s*current_town\s*->\s*owner\s*;"
            r"\s*playerData\s*\*\s*player\s*=\s*&\s*gpGame\s*->\s*players"
            r"\s*\[\s*current_town\s*->\s*owner\s*\]\s*;\s*"
            r"armyGroup\s+hero_army\s*=\s*candidate\s*->\s*army\s*;\s*"
            r"armyGroup\s+town_army\s*=\s*current_town\s*->\s*get_army"
            r"\s*\(\s*\)\s*;\s*type_AI_creature_purchaser\s+purchaser"
            r"\s*\(\s*player_id\s*,\s*current_town\s*\)\s*;"),
        SourceRule(
            "value_of_hiring keeps Dreamcast's resources[7] and "
            "population[14] locals and their save order",
            r"\bint\s+resources\s*\[\s*7\s*\]\s*;\s*memcpy\s*\(\s*"
            r"resources\s*,\s*player\s*->\s*resources\s*,\s*sizeof\s*\(\s*"
            r"resources\s*\)\s*\)\s*;\s*short\s+population\s*\[\s*14"
            r"\s*\]\s*;\s*memcpy\s*\(\s*population\s*,\s*current_town"
            r"\s*->\s*population\s*,\s*sizeof\s*\(\s*population\s*\)"
            r"\s*\)\s*;"),
        SourceRule(
            "value_of_hiring keeps Dreamcast's destinations, monsters, "
            "destination and monster_cell local identities around the two "
            "destination-pricing passes",
            r"std\s*::\s*vector\s*<\s*HeroDestination\s*>\s+destinations"
            r"\s*;.*?std\s*::\s*vector\s*<\s*pathCell\s*\*\s*>\s+monsters"
            r"\s*;\s*long\s+total_value\s*=\s*0\s*;\s*HeroDestination\s+"
            r"destination\s*;\s*pathCell\s*\*\s*monster_cell\s*;\s*"
            r"unsigned\s+int\s+i\s*;\s*for\s*\(\s*i\s*=\s*0\s*;\s*i"
            r"\s*<\s*destinations\s*\.\s*size\s*\(\s*\)\s*;"),
        SourceRule(
            "value_of_hiring keeps Dreamcast's heroes_touched then "
            "best_hero_value initialization statements before the own-hero "
            "pass",
            r"\blong\s+heroes_touched\s*=\s*1\s*;\s*long\s+"
            r"best_hero_value\s*=\s*0\s*;\s*for\s*\(\s*int\s+hero_index"),
        SourceRule(
            "value_of_hiring restores Dreamcast's hero army, resources and "
            "town population snapshots in their recovered order",
            r"candidate\s*->\s*owner\s*=\s*-\s*1\s*;\s*candidate\s*->\s*"
            r"army\s*=\s*hero_army\s*;\s*memcpy\s*\(\s*player\s*->\s*"
            r"resources\s*,\s*resources\s*,\s*sizeof\s*\(\s*resources\s*"
            r"\)\s*\)\s*;\s*memcpy\s*\(\s*current_town\s*->\s*"
            r"population\s*,\s*population\s*,\s*sizeof\s*\(\s*population"
            r"\s*\)\s*\)\s*;"),
    ),
    ("ai_player.obj", 0x2F694): (
        SourceRule(
            "fill_prohibited_array keeps Dreamcast's human_strength, "
            "income[7], short i and resources[7] function locals in "
            "CodeView declaration order",
            r"\A\s*long\s+human_strength\s*;\s*"
            r"int\s+income\s*\[\s*7\s*\]\s*;\s*"
            r"short\s+i\s*;\s*"
            r"int\s+resources\s*\[\s*7\s*\]\s*;"),
        SourceRule(
            "fill_prohibited_array keeps Dreamcast's human_strength zero "
            "statement after the initial dwelling-growth cost pass",
            r"\bget_growth_rate\s*\(.*?\bGetMonsterCost\s*\(.*?"
            r"\bhuman_strength\s*=\s*0\s*;.*?"
            r"\bsum_player_dwellings\s*\("),
    ),
    ("command.obj", 0x6F824): (
        SourceRule(
            "process_first_aid keeps both Dreamcast army::GetName calls",
            r"\bGetName\s*\(\s*\)", 2, 2),
        SourceRule(
            "process_first_aid may not flatten army::GetName into a "
            "creature-trait read",
            r"\bakCreatureTypeTraits\b", 0, 0),
        SourceRule(
            "process_first_aid may not force either Dreamcast GetName "
            "boundary out of line with an inline-depth fence",
            r"#\s*pragma\s+inline_depth\s*\(\s*0\s*\)"
            r"(?:(?!#\s*pragma\s+inline_depth\s*\(\s*\)).)*?"
            r"\bGetName\s*\(\s*\)",
            0, 0, include_directives=True),
    ),
    ("command.obj", 0x6F984): (
        SourceRule(
            "ProcessNextAction keeps all four Dreamcast army::GetName calls",
            r"\bGetName\s*\(\s*\)", 4, 4),
        SourceRule(
            "ProcessNextAction may not flatten army::GetName into a "
            "creature-trait read",
            r"\bakCreatureTypeTraits\b", 0, 0),
        SourceRule(
            "ProcessNextAction may not force a Dreamcast GetName boundary "
            "out of line with an inline-depth fence",
            r"#\s*pragma\s+inline_depth\s*\(\s*0\s*\)"
            r"(?:(?!#\s*pragma\s+inline_depth\s*\(\s*\)).)*?"
            r"\bGetName\s*\(\s*\)",
            0, 0, include_directives=True),
    ),
    ("game.obj", 0xAEB64): (
        SourceRule(
            "loadVictoryCondition keeps Dreamcast's procedure-scope "
            "int_buffer, count and char_buffer locals in raw NB11 order",
            r"\A\s*int\s+int_buffer\s*;\s*"
            r"int\s+count\s*;\s*char\s+char_buffer\s*;"),
        SourceRule(
            "loadVictoryCondition keeps Dreamcast's two leading count/read "
            "assignments and common-flag statement order",
            r"\bcount\s*=\s*infile\s*->\s*Read\s*\(\s*"
            r"&\s*char_buffer\s*,\s*sizeof\s*\(\s*char_buffer\s*\)\s*"
            r"\)\s*;\s*victoryCondition\s*\.\s*AllowNormalVictory\s*"
            r"=\s*char_buffer\s*!=\s*0\s*;\s*"
            r"count\s*=\s*infile\s*->\s*Read\s*\(\s*"
            r"&\s*char_buffer\s*,\s*sizeof\s*\(\s*char_buffer\s*\)\s*"
            r"\)\s*;\s*victoryCondition\s*\.\s*AppliesToComputer\s*"
            r"=\s*char_buffer\s*!=\s*0\s*;"),
    ),
    ("event_record.obj", 0x8CFA8): (
        SourceRule(
            "type_record_show_boat keeps Dreamcast's get_location helper "
            "statement before the destination assignment",
            r"\bprevious_location\s*=\s*_current_boat\s*->\s*"
            r"get_location\s*\(\s*\)\s*;\s*location\s*=\s*"
            r"_location\s*;", 1, 1),
    ),
    ("event_record.obj", 0x8C710): (
        SourceRule(
            "type_record_move_hero keeps Dreamcast's current hero, facing "
            "snapshot, direction, get_location helper and destination "
            "statements in recovered order",
            r"\A\s*current_hero\s*=\s*_hero\s*;\s*"
            r"restore_flag\s*=\s*_hero\s*->\s*facing\s*;\s*"
            r"direction\s*=\s*_direction\s*;\s*source\s*=\s*"
            r"_hero\s*->\s*get_location\s*\(\s*\)\s*;\s*"
            r"destination\s*=\s*_destination\s*;\s*\Z", 1, 1),
    ),
    ("event_record.obj", 0x8CB2C): (
        SourceRule(
            "type_record_claim_mine keeps Dreamcast's id, new-owner and "
            "mine-owner snapshot statements in recovered order",
            r"\A\s*id\s*=\s*_id\s*;\s*new_owner\s*=\s*_new_owner\s*;"
            r"\s*old_owner\s*=\s*gpGame\s*->\s*mines\s*\[\s*_id\s*"
            r"\]\s*\.\s*playerOwner\s*;\s*\Z", 1, 1),
    ),
    ("event_record.obj", 0x8CCFC): (
        SourceRule(
            "type_record_claim_town keeps Dreamcast's id, new-owner and "
            "town-owner snapshot statements in recovered order",
            r"\A\s*id\s*=\s*_id\s*;\s*new_owner\s*=\s*_new_owner\s*;"
            r"\s*old_owner\s*=\s*gpGame\s*->\s*towns\s*\[\s*_id\s*"
            r"\]\s*\.\s*owner\s*;\s*\Z", 1, 1),
    ),
    ("event_record.obj", 0x8DFE0): (
        SourceRule(
            "record_claim_mine keeps Dreamcast's separate mine lookup and "
            "location construction rows in recovered order",
            r"\bmine\s*&\s*(?P<claim_mine_ref>[A-Za-z_]\w*)\s*=\s*"
            r"mines\s*\[\s*id\s*\]\s*;\s*type_point\s+location\s*"
            r"\(\s*(?P=claim_mine_ref)\s*\.\s*mapX\s*,\s*"
            r"(?P=claim_mine_ref)\s*\.\s*mapY\s*,\s*"
            r"(?P=claim_mine_ref)\s*\.\s*mapZ\s*\)\s*;", 1, 1),
        SourceRule(
            "record_claim_mine keeps Dreamcast's stack message, send and "
            "direct claim-record push in recovered order",
            r"\bCMCClaimMine\s+msg\s*\(\s*id\s*,\s*new_owner\s*\)\s*;"
            r"\s*SendMapChange\s*\(\s*&\s*msg\s*\)\s*;\s*"
            r"eventRecords\s*\.\s*push_back\s*\(\s*new\s+"
            r"type_record_claim_mine\s*\(\s*id\s*,\s*new_owner\s*"
            r"\)\s*\)\s*;", 1, 1),
    ),
    ("event_record.obj", 0x8E18C): (
        SourceRule(
            "record_show_boat keeps Dreamcast's direct show-record "
            "construction inside one eventRecords push_back statement",
            r"\beventRecords\s*\.\s*push_back\s*\(\s*new\s+"
            r"type_record_show_boat\s*\(\s*current_boat\s*,\s*point\s*"
            r"\)\s*\)\s*;", 1, 1),
    ),
    ("event_record.obj", 0x8E2F8): (
        SourceRule(
            "record_teleport keeps Dreamcast's direct teleport-record "
            "construction inside one eventRecords push_back statement",
            r"\beventRecords\s*\.\s*push_back\s*\(\s*new\s+"
            r"type_record_teleport\s*\(\s*who\s*,\s*destination\s*"
            r"\)\s*\)\s*;", 1, 1),
    ),
    ("event_record.obj", 0x8E058): (
        SourceRule(
            "record_claim_town keeps Dreamcast's otherwise optimized-away "
            "GetTown validation before message construction, send and "
            "direct record push",
            r"\A\s*GetTown\s*\(\s*id\s*\)\s*;\s*"
            r"CMCClaimTown\s+msg\s*\(\s*id\s*,\s*new_owner\s*\)\s*;"
            r"\s*SendMapChange\s*\(\s*&\s*msg\s*\)\s*;\s*"
            r"eventRecords\s*\.\s*push_back\s*\(\s*new\s+"
            r"type_record_claim_town\s*\(\s*id\s*,\s*new_owner\s*"
            r"\)\s*\)\s*;\s*\Z", 1, 1),
    ),
    ("events.obj", 0x94760): (
        SourceRule(
            "DoEventPrison keeps Dreamcast's THeroID source local under its "
            "heroID name and compatible T_INT4 storage",
            r"\bint\s+heroID\s*=\s*cell\s*->\s*extraInfo\s*;"),
        SourceRule(
            "DoEventPrison keeps Dreamcast's OldColorCycling and "
            "OldAnimCtrPaused save/disable and restore order",
            r"\bunsigned\s+char\s+OldColorCycling\s*=\s*"
            r"gUnnamed67f574\s*;\s*gUnnamed67f574\s*=\s*0\s*;\s*"
            r"unsigned\s+char\s+OldAnimCtrPaused\s*=\s*animCtrPaused\s*;"
            r"\s*animCtrPaused\s*=\s*1\s*;.*?"
            r"\bFizzleCenter\s*\(\s*FIZZLE_SOUND_PICKUP\s*\)\s*;\s*"
            r"animCtrPaused\s*=\s*OldAnimCtrPaused\s*;\s*"
            r"gUnnamed67f574\s*=\s*OldColorCycling\s*;"),
        SourceRule(
            "DoEventPrison keeps Dreamcast's rescued-hero statement order "
            "while permitting Complete-only statements between them",
            r"\brecord_show_hero\s*\(.*?\)\s*;.*?"
            r"prisoner\s*->\s*owner\s*=\s*current_hero\s*->\s*owner\s*;"
            r".*?heroAvailability\s*\[\s*heroID\s*\]\s*=\s*"
            r"current_hero\s*->\s*owner\s*;.*?"
            r"gpCurrentPlayer\s*->\s*heroes\s*\[\s*"
            r"gpCurrentPlayer\s*->\s*numHeroes\s*\]\s*=\s*heroID\s*;"
            r"\s*\+\+\s*gpCurrentPlayer\s*->\s*numHeroes\s*;.*?"
            r"prisoner\s*->\s*x\s*=\s*point\s*\.\s*x\s*;\s*"
            r"prisoner\s*->\s*y\s*=\s*point\s*\.\s*y\s*;\s*"
            r"prisoner\s*->\s*z\s*=\s*point\s*\.\s*z\s*;\s*"
            r"prisoner\s*->\s*flags\s*=\s*0\s*;\s*"
            r"prisoner\s*->\s*facing\s*=\s*hero\s*::\s*kFacingE\s*;"
            r"\s*prisoner\s*->\s*movePoints\s*=\s*prisoner\s*->\s*"
            r"GetMobility\s*\(\s*\)\s*;\s*prisoner\s*->\s*"
            r"maxMovePoints\s*=\s*prisoner\s*->\s*movePoints\s*;\s*"
            r"cell\s*->\s*is_trigger\s*=\s*0\s*;\s*"
            r"cell\s*->\s*type_value\s*=\s*0\s*;\s*"
            r"prisoner\s*->\s*obscure_cell\s*\(\s*\)\s*;"),
    ),
    ("events.obj", 0x9AF34): (
        SourceRule(
            "CombatMonsterEvent keeps Dreamcast's event_seed local and "
            "Demobilize/SRand statement order",
            r"\bDemobilizeCurrHero\s*\(\s*0\s*,\s*1\s*\)\s*;.*?"
            r"\bint\s+event_seed\s*=.*?;\s*SRand\s*\(\s*event_seed\s*"
            r"\)\s*;"),
        SourceRule(
            "CombatMonsterEvent keeps Dreamcast's shared primary-skill then "
            "army-value order and plain double ratio local",
            r"who\s*->\s*get_primary_skill_total\s*\(\s*\)\s*;.*?"
            r"double\s+ratio\s*=\s*who\s*->\s*army\s*\.\s*"
            r"get_AI_value\s*\(\s*\)\s*;\s*ratio\s*/="
            r"\s*combat_value\s*;"),
        SourceRule(
            "CombatMonsterEvent may not replace NB11's plain T_REAL64 ratio "
            "with a volatile codegen probe",
            r"\bvolatile\s+double\s+ratio\b", 0, 0),
        SourceRule(
            "CombatMonsterEvent keeps Dreamcast's army_group local and the "
            "scoped tempNumTroops then tempArmies arrays",
            r"\barmyGroup\s+army_group\s*;.*?"
            r"if\s*\(\s*monType2\s*!=\s*CREATURE_NONE\s*\|\|\s*"
            r"monType3\s*!=\s*CREATURE_NONE\s*\)\s*\{\s*"
            r"int\s+tempNumTroops\s*\[\s*7\s*\]\s*;\s*"
            r"TCreatureType\s+tempArmies\s*\[\s*7\s*\]\s*;"),
    ),
    ("cmbtmgr.obj", 0x5E09C): (
        SourceRule(
            "LoadArmies keeps raw NB11's procedure-scope side local before "
            "the outer loop",
            r"\A\s*int\s+side\s*;\s*for\s*\(\s*side\s*=\s*0\s*;\s*"
            r"side\s*<\s*2\s*;\s*side\s*\+\+\s*\)"),
        SourceRule(
            "LoadArmies keeps raw NB11's const grouped byte and const "
            "layout int locals in record order",
            r"const\s+unsigned\s+char\s+grouped\s*=\s*combat_hero\s*&&"
            r".*?;\s*const\s+int\s+layout\s*=\s*group\s*->\s*"
            r"GetNumArmies\s*\(\s*\)\s*-\s*1\s*;"),
        SourceRule(
            "LoadArmies keeps raw NB11's per-slot hex then army-reference "
            "thisArmy locals and invokes Init/LoadResources through it",
            r"if\s*\(\s*armyGroups\s*\[\s*side\s*\]\s*->\s*armies\s*"
            r"\[\s*i\s*\]\s*==\s*CREATURE_NONE\s*\)\s*continue\s*;\s*"
            r"int\s+hex\s*;\s*army\s*&\s*thisArmy\s*=\s*armies\s*\["
            r"\s*side\s*\]\s*\[\s*placed\s*\]\s*;.*?"
            r"thisArmy\s*\.\s*Init\s*\(.*?\)\s*;\s*"
            r"thisArmy\s*\.\s*LoadResources\s*\(\s*\)\s*;"),
    ),
    ("cmbtmgr.obj", 0x5F518): (
        SourceRule(
            "NextArmy keeps Complete's field_4f0 guard around the "
            "Dreamcast army::IsIncapacitated helper boundary",
            r"if\s*\(\s*stack\s*->\s*field_4f0\s*&&\s*stack\s*->\s*"
            r"IsIncapacitated\s*\(\s*\)\s*\)\s*continue\s*;"),
    ),
    ("cmbtmgr.obj", 0x5F934): (
        SourceRule(
            "SetNextArmy keeps both Dreamcast army::get_controlling_side "
            "source calls instead of a file-local expansion",
            r"stack\s*->\s*get_controlling_side\s*\(\s*\)", 2, 2),
        SourceRule(
            "SetNextArmy keeps both Dreamcast army::GetName source calls "
            "instead of a direct creature-trait expansion",
            r"stack\s*->\s*GetName\s*\(\s*\)", 2, 2),
        SourceRule(
            "SetNextArmy keeps raw NB11's sole result local in the wraith "
            "sample scope and the recovered message/helper order",
            r"case\s+army\s*::\s*ARMY_CREATURE_WRAITH\s*:.*?"
            r"if\s*\(\s*!\s*IsQuickCombat\s*\(\s*\)\s*\)\s*\{\s*"
            r"SAMPLE2\s+sample\s*=\s*LoadPlaySample\s*\(.*?\)\s*;\s*"
            r"std\s*::\s*string\s+result\s*;.*?"
            r"if\s*\(\s*stack\s*->\s*numTroops\s*==\s*1\s*\).*?"
            r"result\s*=\s*format_string\s*\(.*?stack\s*->\s*GetName"
            r"\s*\(\s*\).*?else\s*\{.*?std\s*::\s*string\s+many\s*="
            r"\s*format_string\s*\(.*?stack\s*->\s*GetName\s*\(\s*\)"
            r".*?result\s*\.\s*assign\s*\(.*?\)\s*;.*?"
            r"combatWindow\s*->\s*combat_message\s*\(\s*result\s*\.\s*"
            r"c_str\s*\(\s*\)\s*,\s*1\s*,\s*0\s*\)\s*;.*?"
            r"SpellEffect\s*\(.*?\)\s*;\s*WaitEndSample\s*\("),
        SourceRule(
            "SetNextArmy keeps Dreamcast's lastMovedArmy clear immediately "
            "before the named GetControl tail call",
            r"lastMovedArmy\s*=\s*0\s*;\s*GetControl\s*\(\s*\)\s*;"),
    ),
    ("bottomviewsubwindow.obj", 0x55DF4): (
        SourceRule(
            "TBottomViewTown keeps all seven Dreamcast town::HasBuilding "
            "source calls",
            r"\bwhich\s*->\s*HasBuilding\s*\(", 7, 7),
        SourceRule(
            "TBottomViewTown keeps NB11's procedure-scope copy-initialized "
            "town_size_name local",
            r"\A(?:(?!\{).)*?std\s*::\s*string\s+town_size_name\s*=\s*"
            r"gTownSizeNames\s*\[\s*hallLevel\s*\]\s*;"),
        SourceRule(
            "TBottomViewTown keeps Dreamcast's hall, town_size_name, fort "
            "and silo helper order and each recovered check_included flag",
            r"\bHasBuilding\s*\(\s*HALL_TOWN_ID\s*,\s*0\s*\).*?"
            r"\bHasBuilding\s*\(\s*HALL_CITY_ID\s*,\s*0\s*\).*?"
            r"\bHasBuilding\s*\(\s*HALL_CAPITOL_ID\s*,\s*0\s*\).*?"
            r"std\s*::\s*string\s+town_size_name\s*=\s*"
            r"gTownSizeNames\s*\[\s*hallLevel\s*\]\s*;.*?"
            r"\bHasBuilding\s*\(\s*CASTLE_FORT_ID\s*,\s*0\s*\).*?"
            r"\bHasBuilding\s*\(\s*CASTLE_CITADEL_ID\s*,\s*0\s*\).*?"
            r"\bHasBuilding\s*\(\s*CASTLE_CASTLE_ID\s*,\s*0\s*\).*?"
            r"\bHasBuilding\s*\(\s*MARKETPLACE_SILO_ID\s*,\s*1\s*\)"),
        SourceRule(
            "TBottomViewTown keeps NB11's scoped resource local immediately "
            "after the silo HasBuilding guard and before its indexed sweep",
            r"if\s*\(\s*which\s*->\s*HasBuilding\s*\(\s*"
            r"MARKETPLACE_SILO_ID\s*,\s*1\s*\)\s*\)\s*\{\s*"
            r"int\s*\*\s*resource\s*=\s*which\s*->\s*get_silo_income"
            r"\s*\(\s*\)\s*;.*?\bresource\s*\[\s*i\s*\]"),
        SourceRule(
            "TBottomViewTown may not flatten Dreamcast's silo HasBuilding "
            "boundary into the active bitset",
            r"which\s*->\s*active\s*&\s*bitNumber\s*\[\s*"
            r"MARKETPLACE_SILO_ID\s*\]", 0, 0),
    ),
    ("combatresultswindow.obj", 0x68364): (
        SourceRule(
            "TCombatResultsWindow keeps Dreamcast's function-scope amount, "
            "type, loss arrays, const selected-hero pointer, totals, text "
            "and firstX "
            "locals in CodeView declaration order",
            r"\A\s*gpCombatResultsWindow\s*=\s*this\s*;\s*"
            r"long\s+amount\s*;\s*TCreatureType\s+type\s*;\s*"
            r"int\s+iDeadArmyTypes\s*\[\s*2\s*\]\s*\[\s*20\s*\]\s*;\s*"
            r"int\s+iDeadArmyNumTroops\s*\[\s*2\s*\]\s*\[\s*20\s*\]"
            r"\s*;\s*const\s+hero\s*\*\s*const\s+my_hero\s*=.*?;\s*"
            r"int\s+iTtlDeadArmies\s*\[\s*2\s*\]\s*;\s*"
            r"char\s+cText\s*\[\s*100\s*\]\s*;\s*"
            r"int\s+firstX\s*;"),
        SourceRule(
            "TCombatResultsWindow keeps all eighteen Dreamcast "
            "TTextResource::operator[] source calls",
            r"\(\s*\*\s*gpGeneralText\s*\)\s*\[", 18, 18),
        SourceRule(
            "TCombatResultsWindow may not de-inline Dreamcast "
            "TTextResource::operator[] calls into direct GetText calls",
            r"gpGeneralText\s*->\s*GetText\s*\(", 0, 0),
        SourceRule(
            "TCombatResultsWindow keeps Dreamcast's cTemp[150] local in the "
            "my_hero sprintf scope",
            r"if\s*\(\s*my_hero\s*\)\s*\{\s*"
            r"char\s+cTemp\s*\[\s*150\s*\]\s*;\s*sprintf\s*\("),
        SourceRule(
            "TCombatResultsWindow keeps both scoped Dreamcast numMons "
            "locals",
            r"\bint\s+numMons\s*=\s*0\s*;", 2, 2),
        SourceRule(
            "TCombatResultsWindow keeps both Dreamcast positive "
            "strongest-stack scopes around Complete's arrow-tower test",
            r"if\s*\(\s*stack\.creatureType\s*!=\s*-\s*1\s*&&\s*"
            r"stack\.creatureType\s*!=\s*CREATURE_ARROW_TOWER\s*\)\s*"
            r"\{\s*numMons\s*\+\+\s*;", 2, 2),
        SourceRule(
            "TCombatResultsWindow keeps Dreamcast's iMaxToShow local at the "
            "min statement",
            r"\bint\s+iMaxToShow\s*=\s*min\s*\("),
        SourceRule(
            "TCombatResultsWindow keeps Dreamcast's inner x local",
            r"\bint\s+x\s*=\s*firstX\s*\+\s*42\s*\*"),
        SourceRule(
            "TCombatResultsWindow keeps Dreamcast line 280's one positive "
            "loss-aggregation scope",
            r"if\s*\(\s*creature\s*!=\s*-\s*1\s*&&\s*lost\s*>\s*0\s*"
            r"\)\s*\{\s*int\s+row\s*;"),
        SourceRule(
            "TCombatResultsWindow keeps all twenty-two Dreamcast "
            "Widgets.push_back source calls",
            r"\bWidgets\s*\.\s*push_back\s*\(", 22, 22),
        SourceRule(
            "TCombatResultsWindow keeps Dreamcast line 319's accept-box "
            "allocation, construction and push in one statement group",
            r"Widgets\s*\.\s*push_back\s*\(\s*new\s+bitmapBorder\s*\(\s*"
            r"384\s*,\s*506\s*,\s*66\s*,\s*32\s*,.*?"
            r"0x800\s*\)\s*\)\s*;"),
        SourceRule(
            "TCombatResultsWindow keeps source-gap line 320's zero-emission "
            "accept initializer before line 321's construction assignment",
            r"button\s*\*\s*accept\s*=\s*0\s*;\s*"
            r"accept\s*=\s*new\s+button\s*\("),
        SourceRule(
            "TCombatResultsWindow keeps both Dreamcast button::set_hotkey "
            "source calls",
            r"\bset_hotkey\s*\(", 2, 2),
        SourceRule(
            "TCombatResultsWindow keeps Dreamcast's source-visible min call",
            r"(?<![_\w])min\s*\(", 1, 1),
    ),
    ("philai.obj", 0x10D47C): (
        SourceRule(
            "CheckDoMain keeps Dreamcast's one-time frame-rate timer flag "
            "guard, bit set and initial GameTime::Get statement order",
            r"\A\s*if\s*\(\s*!\s*\(\s*gUnnamed69cca4\s*&\s*1\s*\)\s*"
            r"\)\s*\{\s*gUnnamed69cca4\s*\|=\s*1\s*;\s*"
            r"iLastFrameRateTimer\s*=\s*GameTime\s*::\s*Get\s*"
            r"\(\s*\)\s*;\s*\}"),
        SourceRule(
            "CheckDoMain keeps Dreamcast's elapsed-or-animation-deadline "
            "guard followed by Process1WindowsMessage then PollSound",
            r"if\s*\(\s*GameTime\s*::\s*ElapsedSince\s*\(\s*"
            r"iLastFrameRateTimer\s*\)\s*>\s*15\s*\|\|\s*"
            r"GameTime\s*::\s*IsPast\s*\(\s*glTimers\s*\[\s*"
            r"GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT\s*\]\s*\)\s*\)\s*"
            r"\{\s*Process1WindowsMessage\s*\(\s*\)\s*;\s*"
            r"PollSound\s*\(\s*\)\s*;"),
        SourceRule(
            "CheckDoMain keeps Dreamcast's nested animation-deadline scope, "
            "bMouseOnly cursor clear, +180 deadline and final frame-rate "
            "timer refresh in recovered order",
            r"PollSound\s*\(\s*\)\s*;\s*if\s*\(\s*GameTime\s*::\s*"
            r"IsPast\s*\(\s*glTimers\s*\[\s*"
            r"GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT\s*\]\s*\)\s*\)\s*"
            r"\{\s*if\s*\(\s*!\s*bMouseOnly\s*\)\s*"
            r"bSpecialHideCursor\s*=\s*0\s*;\s*glTimers\s*\[\s*"
            r"GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT\s*\]\s*=\s*"
            r"GameTime\s*::\s*Get\s*\(\s*\)\s*\+\s*180\s*;\s*\}\s*"
            r"iLastFrameRateTimer\s*=\s*GameTime\s*::\s*Get\s*"
            r"\(\s*\)\s*;"),
    ),
    ("philai.obj", 0x10DEA8): (
        SourceRule(
            "value_of_war_factory keeps Dreamcast lines 525-530 as the "
            "siege-artifact conversion followed by a typed creature-cost "
            "row before the resource accumulator",
            r"\bTCreatureType\s+creature\s*=\s*"
            r"siege_artifact_to_creature\s*\(\s*engine\s*\)\s*;.*?"
            r"akCreatureTypeTraits\s*\[\s*creature\s*\]\s*\.\s*cost"
            r"[^;]*;\s*long\s+resource_cost\s*=\s*0\s*;"),
        SourceRule(
            "value_of_war_factory may not flatten Dreamcast's typed "
            "TCreatureTypeTraits cost row into the raw record view",
            r"\bgCreatureRecords\b", 0, 0),
    ),
    ("philai.obj", 0x10E3F8): (
        SourceRule(
            "AI_enter_town keeps Dreamcast's nested Grail possession, "
            "building-absence and legality guards around the remove, build, "
            "victory-check and end-game helper sequence",
            r"if\s*\(\s*current_hero\s*->\s*HasArtifact\s*\(\s*"
            r"ARTIFACT_HOLY_GRAIL\s*\)\s*&&\s*!\s*current_town\s*->\s*"
            r"HasBuilding\s*\(\s*HOLY_GRAIL_ID\s*,\s*0\s*\)\s*&&\s*"
            r"current_town\s*->\s*is_legal_building\s*\(\s*"
            r"HOLY_GRAIL_ID\s*\)\s*\)\s*\{\s*current_hero\s*->\s*"
            r"remove_artifact\s*\(\s*ARTIFACT_HOLY_GRAIL\s*\)\s*;\s*"
            r"current_town\s*->\s*BuildBuilding\s*\(\s*HOLY_GRAIL_ID\s*,"
            r"\s*1\s*,\s*1\s*\)\s*;\s*if\s*\(\s*gpGame\s*->\s*"
            r"mapHeader\s*\.\s*victoryCondition\s*\.\s*"
            r"CheckForGrailBuildingWin\s*\(\s*\)\s*\)\s*"
            r"CheckEndGame\s*\(\s*0\s*\)\s*;\s*\}"),
        SourceRule(
            "AI_enter_town keeps raw NB11's sole artifact local inside the "
            "spellbook-purchase scope, with construction, GiveArtifact and "
            "gold debit in Dreamcast statement order",
            r"if\s*\(\s*player\s*->\s*resources\s*\[\s*GOLD\s*\]\s*"
            r">=\s*500\s*&&\s*current_town\s*->\s*HasBuilding\s*\(\s*"
            r"MAGE_GUILD_ID\s*,\s*1\s*\)\s*&&\s*!\s*current_hero\s*->\s*"
            r"IsWieldingArtifact\s*\(\s*ARTIFACT_SPELLBOOK\s*\)\s*\)\s*"
            r"\{\s*type_artifact\s+artifact\s*\(\s*ARTIFACT_SPELLBOOK\s*,"
            r"\s*-\s*1\s*\)\s*;\s*current_hero\s*->\s*GiveArtifact\s*"
            r"\(\s*&\s*artifact\s*,\s*1\s*,\s*1\s*\)\s*;\s*"
            r"player\s*->\s*resources\s*\[\s*GOLD\s*\]\s*-=\s*500\s*;"
            r"\s*\}"),
        SourceRule(
            "AI_enter_town keeps Dreamcast's upgrade_creatures then "
            "buy_special_building order and the difficulty-scoped pair of "
            "AI_swap_artifacts calls before both siege-engine sites",
            r"\bupgrade_creatures\s*\(\s*current_hero\s*,\s*current_town\s*"
            r"\)\s*;.*?\bbuy_special_building\s*\(\s*current_hero\s*,\s*"
            r"current_town\s*\)\s*;.*?if\s*\(\s*gpGame\s*->\s*setup\s*\."
            r"\s*difficulty\s*\)\s*\{.*?\bAI_swap_artifacts\s*\(\s*"
            r"current_hero\s*,\s*garrison_hero\s*\)\s*;\s*"
            r"AI_swap_artifacts\s*\(\s*garrison_hero\s*,\s*current_hero\s*"
            r"\)\s*;.*?\bbuy_siege_engine\s*\(\s*current_hero\s*,\s*"
            r"current_town\s*,\s*BLACKSMITH_ID\s*,.*?\)\s*;\s*"
            r"if\s*\(\s*current_town\s*->\s*type\s*==\s*TOWN_STRONGHOLD"
            r"\s*\)\s*buy_siege_engine\s*\(\s*current_hero\s*,\s*"
            r"current_town\s*,\s*EXTRA_1_ID\s*,\s*ARTIFACT_BALLISTA\s*\)"
            r"\s*;\s*\}"),
        SourceRule(
            "AI_enter_town keeps Dreamcast's DemobilizeCurrHero before the "
            "garrison effect and its final nested GetHero then "
            "ApplySpecialBuildingEffect statement",
            r"gpAdvManager\s*->\s*DemobilizeCurrHero\s*\(\s*0\s*,\s*0\s*"
            r"\)\s*;\s*if\s*\(\s*garrison_hero\s*\)\s*current_town\s*->"
            r"\s*ApplySpecialBuildingEffect\s*\(\s*garrison_hero\s*\)\s*;"
            r"\s*if\s*\(\s*current_town\s*->\s*visitingHeroId\s*!=\s*"
            r"-\s*1\s*\)\s*current_town\s*->\s*"
            r"ApplySpecialBuildingEffect\s*\(\s*gpGame\s*->\s*GetHero\s*"
            r"\(\s*current_town\s*->\s*visitingHeroId\s*\)\s*\)\s*;"),
    ),
    ("philai.obj", 0x10D684): (
        SourceRule(
            "upgrade_creatures keeps Dreamcast's function-scope difference, "
            "amount, dwelling, upgrade_cost and base_cost locals in CodeView "
            "declaration order",
            r"\A\s*int\s+difference\s*\[\s*NUM_RESOURCES\s*\]\s*;\s*"
            r"long\s+amount\s*;\s*long\s+dwelling\s*;\s*const\s+int\s*"
            r"\*\s*upgrade_cost\s*;\s*const\s+int\s*\*\s*base_cost\s*;"
            r"\s*for\s*\(\s*dwelling\s*="),
        SourceRule(
            "upgrade_creatures keeps Dreamcast's base-cost, upgrade-cost, "
            "then amount statement order while retaining Complete's "
            "retail-proven widened cost rows",
            r"\bbase_cost\s*=\s*gCreatureRecords\b[^;]*;\s*"
            r"upgrade_cost\s*=\s*gCreatureRecords\b[^;]*;\s*"
            r"amount\s*=\s*current_hero\s*->\s*army\s*\.\s*numTroops\s*"
            r"\[\s*slot\s*\]\s*;"),
        SourceRule(
            "upgrade_creatures passes Dreamcast line 221's integer dwelling "
            "expression directly to HasBuilding without a non-attested enum "
            "bridge",
            r"\bHasBuilding\s*\(\s*DWELLING_0_UPG_ID\s*\+\s*dwelling\s*,"
            r"\s*1\s*\)"),
    ),
    ("philai.obj", 0x11105C): (
        SourceRule(
            "value_of_enemy_town keeps Dreamcast's procedure-scope "
            "creature_cost, include_growth and creature locals in CodeView "
            "record order",
            r"\A(?:(?!\{).)*?\bint\s+creature_cost\s*\[\s*"
            r"NUM_RESOURCES\s*\]\s*;"
            r"(?:(?!\{).)*?\bunsigned\s+char\s+include_growth\s*;"
            r"(?:(?!\{).)*?\bTCreatureType\s+creature\s*;"),
    ),
    ("game.obj", 0xA3E5C): (
        SourceRule(
            "LoadMinePool retains Dreamcast's signed int x local; an exact "
            "unsigned spelling is byte proof but not source-shape closure",
            r"(?m)^[ \t]*int[ \t]+x[ \t]*;"),
    ),
    ("game.obj", 0xA46E8): (
        SourceRule(
            "LoadBoatPool keeps Dreamcast's function-scope ushort_buffer, "
            "count, x, uchar_buffer and char_buffer locals in CodeView "
            "record order",
            r"\A\s*unsigned\s+short\s+ushort_buffer\s*;\s*"
            r"int\s+count\s*;\s*int\s+x\s*;\s*"
            r"unsigned\s+char\s+uchar_buffer\s*;\s*"
            r"char\s+char_buffer\s*;"),
        SourceRule(
            "LoadBoatPool keeps every Dreamcast read result in count",
            r"\bcount\s*=\s*infile\s*->\s*Read\s*\(", 8, 8),
        SourceRule(
            "LoadBoatPool keeps Dreamcast's two uchar, five char and one "
            "ushort typed read buffers",
            r"\bcount\s*=\s*infile\s*->\s*Read\s*\(\s*&\s*"
            r"uchar_buffer\b", 2, 2),
        SourceRule(
            "LoadBoatPool keeps Dreamcast's five char-buffer reads",
            r"\bcount\s*=\s*infile\s*->\s*Read\s*\(\s*&\s*"
            r"char_buffer\b", 5, 5),
        SourceRule(
            "LoadBoatPool keeps Dreamcast's one ushort-buffer read",
            r"\bcount\s*=\s*infile\s*->\s*Read\s*\(\s*&\s*"
            r"ushort_buffer\b", 1, 1),
        SourceRule(
            "LoadBoatPool keeps Dreamcast's typed boat-field assignment "
            "sequence",
            r"\ballocated\s*=\s*char_buffer\s*!=\s*0\s*;.*?"
            r"\bid\s*=\s*uchar_buffer\s*;.*?"
            r"\btype\s*=\s*char_buffer\s*;.*?"
            r"\bfacing\s*=\s*char_buffer\s*;.*?"
            r"\bplayerOwner\s*=\s*char_buffer\s*;.*?"
            r"\boccupying_hero\s*=\s*ushort_buffer\s*;.*?"
            r"\boccupied\s*=\s*char_buffer\s*!=\s*0\s*;"),
    ),
    ("game.obj", 0xA4980): (
        SourceRule(
            "SaveBoatPool keeps Dreamcast's function-scope ushort_buffer, "
            "count, x, uchar_buffer and char_buffer locals in CodeView "
            "record order",
            r"\A\s*unsigned\s+short\s+ushort_buffer\s*;\s*"
            r"int\s+count\s*;\s*int\s+x\s*;\s*"
            r"unsigned\s+char\s+uchar_buffer\s*;\s*"
            r"char\s+char_buffer\s*;"),
        SourceRule(
            "SaveBoatPool keeps every Dreamcast write result in count",
            r"\bcount\s*=\s*outfile\s*->\s*Write\s*\(", 8, 8),
        SourceRule(
            "SaveBoatPool keeps Dreamcast's two uchar, five char and one "
            "ushort typed write buffers",
            r"\bcount\s*=\s*outfile\s*->\s*Write\s*\(\s*&\s*"
            r"uchar_buffer\b", 2, 2),
        SourceRule(
            "SaveBoatPool keeps Dreamcast's five char-buffer writes",
            r"\bcount\s*=\s*outfile\s*->\s*Write\s*\(\s*&\s*"
            r"char_buffer\b", 5, 5),
        SourceRule(
            "SaveBoatPool keeps Dreamcast's one ushort-buffer write",
            r"\bcount\s*=\s*outfile\s*->\s*Write\s*\(\s*&\s*"
            r"ushort_buffer\b", 1, 1),
    ),
    ("game.obj", 0xA4EE8): (
        SourceRule(
            "add_garrison_hero keeps Dreamcast's procedure-scope i, "
            "our_hero and found locals in raw NB11 order",
            r"\A\s*int\s+i\s*;\s*hero\s*\*\s*our_hero\s*;\s*"
            r"int\s+found\s*;\s*if\s*\(\s*our_town\s*->\s*"
            r"visitingHeroId\s*<\s*0\s*\)"),
        SourceRule(
            "add_garrison_hero keeps Dreamcast's GetHero, Merge, "
            "record_hide_hero and scoped CMCHideHero/SendMapChange order",
            r"our_hero\s*=\s*gpGame\s*->\s*GetHero\s*\(\s*our_town\s*"
            r"->\s*visitingHeroId\s*\)\s*;\s*if\s*\(\s*!\s*our_hero\s*"
            r"->\s*army\s*\.\s*Merge\s*\(.*?get_army\s*\(\s*\)\s*"
            r"\)\s*\)\s*\)\s*return\s+0\s*;\s*gpGame\s*->\s*"
            r"record_hide_hero\s*\(\s*our_hero\s*,\s*our_hero\s*->\s*"
            r"owner\s*,\s*0\s*\)\s*;\s*if\s*\(\s*bVideoPaused\s*\)"
            r"\s*\{\s*CMCHideHero\s+hideHero\s*\(\s*our_hero\s*->\s*"
            r"id\s*\)\s*;\s*SendMapChange\s*\(\s*&\s*hideHero\s*\)"
            r"\s*;\s*\}"),
        SourceRule(
            "add_garrison_hero keeps Dreamcast's found assignment, "
            "restore_cell statement and shared i roster-shift loop",
            r"found\s*=\s*FindHero\s*\(\s*our_hero\s*->\s*id\s*\)\s*;"
            r"\s*our_hero\s*->\s*restore_cell\s*\(\s*\)\s*;\s*for\s*"
            r"\(\s*i\s*=\s*found\s*;\s*i\s*<\s*numHeroes\s*-\s*1"
            r"\s*;\s*\+\+\s*i\s*\)\s*heroes\s*\[\s*i\s*\]\s*="
            r"\s*heroes\s*\[\s*i\s*\+\s*1\s*\]\s*;"),
        SourceRule(
            "add_garrison_hero keeps Dreamcast's current-hero clear before "
            "the roster decrement and final town-id stores",
            r"if\s*\(\s*currHeroId\s*==\s*our_hero\s*->\s*id\s*\)\s*"
            r"\{\s*currHeroId\s*=\s*-\s*1\s*;.*?\}\s*--\s*numHeroes"
            r"\s*;\s*our_town\s*->\s*garrisonHeroId\s*=\s*our_hero\s*"
            r"->\s*id\s*;\s*our_town\s*->\s*visitingHeroId\s*=\s*"
            r"-\s*1\s*;\s*return\s+1\s*;"),
    ),
    ("game.obj", 0xA55A8): (
        SourceRule(
            "playerData::save keeps Dreamcast's function-scope uint buffer, "
            "int buffer, write count, loop x, unsigned-char buffer and "
            "char buffer in CodeView declaration order",
            r"\A\s*unsigned\s+long\s+flags\s*;\s*int\s+number\s*;\s*"
            r"int\s+count\s*;\s*int\s+x\s*;\s*unsigned\s+char\s+flag\s*;"
            r"\s*char\s+value\s*;"),
        SourceRule(
            "playerData::save keeps all twenty Dreamcast write results in "
            "the named count local",
            r"\bcount\s*=\s*outfile\s*->\s*Write\s*\(", 20, 20),
        SourceRule(
            "playerData::save reuses Dreamcast's x local for all three "
            "original serialization loops",
            r"for\s*\(\s*x\s*=", 3, 3),
    ),
    ("game.obj", 0xA6350): (
        SourceRule(
            "SetupPuzzlePieces keeps Dreamcast's piece, two percentage "
            "floats, i, j, iExtraPieces and iPiecesRemoved locals in "
            "CodeView declaration order",
            r"\A\s*long\s+piece\s*;\s*"
            r"float\s+fPercentObelisksFound\s*;\s*"
            r"float\s+fPercentExtraPieces\s*;\s*"
            r"int\s+i\s*;\s*long\s+j\s*;\s*"
            r"int\s+iExtraPieces\s*;\s*"
            r"int\s+iPiecesRemoved\s*;"),
    ),
    ("game.obj", 0xA6CD4): (
        SourceRule(
            "GetNewHeroId keeps Dreamcast's compatible hero_class, "
            "total_count, choice, counts, hero_id, weights and "
            "aligned_count procedure locals in raw NB11 order and at "
            "function lifetime; retail VC6 requires int storage for the two "
            "Dreamcast enum identities",
            r"\A\s*int\s+hero_class\s*;\s*long\s+total_count\s*;\s*"
            r"long\s+choice\s*=\s*0\s*;\s*long\s+counts\s*\[\s*18\s*\]"
            r"\s*;\s*int\s+hero_id\s*;\s*long\s+weights\s*\[\s*18\s*\]"
            r"\s*;\s*long\s+aligned_count\s*;"),
        SourceRule(
            "GetNewHeroId keeps Dreamcast's excluded-class suppression and "
            "prefer_alignment arm as distinct statement groups using the "
            "recovered parameter and aligned_count identities",
            r"if\s*\(\s*excluded\s*<\s*kNumHeroClasses\s*&&\s*"
            r"counts\s*\[\s*excluded\s*\]\s*<\s*total_count\s*\)\s*"
            r"\{\s*weights\s*\[\s*excluded\s*\]\s*=\s*0\s*;\s*\}"
            r"\s*if\s*\(\s*prefer_alignment\s*\)\s*\{\s*"
            r"aligned_count\s*=\s*0\s*;"),
        SourceRule(
            "GetNewHeroId keeps Dreamcast's shared choice local across the "
            "weighted class selection and the later hero selection phases",
            r"choice\s*=\s*Random\s*\(\s*1\s*,\s*totalWeight\s*\)\s*;"
            r".*?choice\s*-=\s*weights\s*\[\s*hero_class\s*\]\s*;"
            r".*?choice\s*=\s*Random\s*\(\s*1\s*,\s*counts\s*\[\s*"
            r"hero_class\s*\]\s*\)\s*;.*?--\s*choice\s*==\s*0"),
    ),
    ("game.obj", 0xAC048): (
        SourceRule(
            "randomize_university keeps Dreamcast's university aggregate "
            "identity on Complete's retail-compatible four-int raw record",
            r"\A\s*int\s+university\s*\[\s*4\s*\]\s*;"),
        SourceRule(
            "randomize_university keeps Dreamcast's shared choice, i and "
            "TSecondarySkill skill locals in recovered order while allowing "
            "Complete-only locals between them",
            r"\blong\s+choice\s*;.*?\blong\s+i\s*;.*?"
            r"\bTSecondarySkill\s+skill\s*;"),
    ),
    ("game.obj", 0xB1230): (
        SourceRule(
            "ClaimTown keeps raw NB11's thisTown, old_owner and shared i "
            "procedure locals in recovered name, type and declaration order",
            r"\A\s*town\s*\*\s*thisTown\s*=\s*&\s*towns\s*\[\s*"
            r"townId\s*\]\s*;\s*long\s+old_owner\s*=\s*thisTown\s*->\s*"
            r"owner\s*;\s*long\s+i\s*;"),
        SourceRule(
            "ClaimTown keeps Dreamcast's IsComputerTeam source boundary "
            "inside the nonnegative old-team arm",
            r"if\s*\(\s*team\s*>=\s*0\s*&&\s*IsComputerTeam\s*\(\s*"
            r"team\s*\)\s*\)"),
        SourceRule(
            "ClaimTown reuses raw NB11's sole i local for both generator "
            "sweeps instead of splitting their lifetimes",
            r"for\s*\(\s*i\s*=\s*0\s*;\s*i\s*<\s*generators\s*\.\s*"
            r"size\s*\(\s*\)\s*;\s*i\+\+\s*\)", 2, 2),
    ),
    ("game.obj", 0xB41E0): (
        SourceRule(
            "PerWeek keeps Dreamcast's ten procedure-scope locals in raw "
            "NB11 record order",
            r"\A\s*hero\s*\*\s*obscuring_hero\s*;\s*"
            r"int\s+iAlign\s*;\s*"
            r"TCreatureType\s+alternate_bonus\s*;\s*"
            r"long\s+bonus_amount\s*;\s*"
            r"int\s+x\s*;\s*int\s+y\s*;\s*int\s+i\s*;\s*"
            r"int\s+z\s*;\s*TCreatureType\s+bonus_creature\s*;\s*"
            r"NewmapCell\s*\*\s*map_cell\s*;"),
        SourceRule(
            "PerWeek keeps Dreamcast's iCount and iIncrease locals inside "
            "the MONSTER growth scope",
            r"case\s+MONSTER\s*:\s*\{\s*if\s*\([^;]*?\)\s*\{\s*"
            r"int\s+iCount\s*=.*?;\s*int\s+iIncrease\s*="),
        SourceRule(
            "PerWeek keeps Dreamcast's luck_bonus local inside the "
            "FOUNTAIN_OF_FORTUNE scope",
            r"case\s+FOUNTAIN_OF_FORTUNE\s*:\s*\{\s*"
            r"int\s+luck_bonus\s*=\s*Random\s*\("),
        SourceRule(
            "PerWeek keeps Dreamcast's currHero local inside the weekly "
            "hero-flag loop",
            r"for\s*\(\s*i\s*=\s*0\s*;\s*i\s*<\s*HERO_COUNT.*?\)\s*"
            r"\{\s*hero\s*\*\s*currHero\s*=\s*&\s*heroes\s*\[\s*i\s*\]"),
        SourceRule(
            "PerWeek keeps the Dreamcast-proven IsCastle helper boundary "
            "when Complete folds GiveTroopsToNeutralTowns into the caller",
            r"\btowns\s*\[\s*i\s*\]\s*\.\s*IsCastle\s*\(\s*\)"),
        SourceRule(
            "PerWeek keeps Complete retail's built-mask Summoning Portal "
            "test rather than the active-mask spelling",
            r"current_town\s*->\s*HasBuilding\s*\(\s*EXTRA_1_ID\s*,\s*0\s*\)"),
    ),
    ("game.obj", 0xBCC40): (
        SourceRule(
            "town::IsCastle keeps Dreamcast line 338's three ordered "
            "HasBuilding helper calls",
            r"return\s+HasBuilding\s*\(\s*CASTLE_FORT_ID\s*,\s*0\s*\)"
            r"\s*\|\|\s*HasBuilding\s*\(\s*CASTLE_CITADEL_ID\s*,\s*0\s*\)"
            r"\s*\|\|\s*HasBuilding\s*\(\s*CASTLE_CASTLE_ID\s*,\s*0\s*\)"
            r"\s*;"),
    ),
    ("game.obj", 0xBCCB4): (
        SourceRule(
            "town::IsCapitol keeps Dreamcast line 343's HasBuilding helper "
            "boundary and unsigned-byte result expression",
            r"\A\s*return\s+HasBuilding\s*\(\s*HALL_CAPITOL_ID\s*,\s*0"
            r"\s*\)\s*;\s*\Z"),
    ),
    ("town.obj", 0x1664B0): (
        SourceRule(
            "initialize_hordes keeps Dreamcast lines 954/957/958/959/960's "
            "base dwelling, upgraded slot, creature, dwelling and bonus "
            "statement order",
            r"\beffect\s*->\s*dwelling\s*=\s*slot\s*;.*?"
            r"\bslot\s*\+=\s*TOWN_DWELLING_COUNT\s*;.*?"
            r"\beffect\s*\[\s*1\s*\]\s*\.\s*creature\s*=.*?;.*?"
            r"\beffect\s*\[\s*1\s*\]\s*\.\s*dwelling\s*=\s*slot\s*;"
            r".*?\beffect\s*\[\s*1\s*\]\s*\.\s*bonus\s*="),
    ),
    ("town.obj", 0x1665A0): (
        SourceRule(
            "GiveSpells keeps Dreamcast lines 992/994/999's current-hero, "
            "spellbook and Mage Guild guards as three nested source scopes",
            r"if\s*\(\s*currentHero\s*\)\s*\{\s*"
            r"if\s*\(\s*currentHero\s*->\s*IsWieldingArtifact\s*\(\s*"
            r"ARTIFACT_SPELLBOOK\s*\)\s*\)\s*\{\s*"
            r"if\s*\(\s*HasBuilding\s*\(\s*MAGE_GUILD_ID\s*,\s*1\s*"
            r"\)\s*\)\s*\{"),
    ),
    ("town.obj", 0x166864): (
        SourceRule(
            "SwapHeroes keeps Dreamcast's two resident GetHero statements, "
            "the intervening exchange group and FindHero in source order",
            r"GetHero\s*\([^;]*garrisonHeroId[^;]*\)\s*;.*?"
            r"GetHero\s*\([^;]*visitingHeroId[^;]*\)\s*;.*?"
            r"std\s*::\s*swap\s*\([^;]*garrisonHeroId\s*,[^;]*"
            r"visitingHeroId\s*\)\s*;.*?FindHero\s*\([^;]*->\s*id\s*"
            r"\)\s*;"),
        SourceRule(
            "SwapHeroes keeps Dreamcast's record_hide_hero, restore_cell, "
            "CMCHideHero construction and SendMapChange statement group",
            r"record_hide_hero\s*\([^;]*\)\s*;\s*[^;]*->\s*"
            r"restore_cell\s*\(\s*\)\s*;\s*CMCHideHero\s+\w+\s*\("
            r"[^;]*->\s*id\s*\)\s*;\s*SendMapChange\s*\(\s*&\s*\w+"
            r"\s*\)\s*;"),
        SourceRule(
            "SwapHeroes keeps Dreamcast's roster-shift loop before the "
            "count decrement and vacated-slot sentinel",
            r"for\s*\([^;]*;[^;]*numHeroes\s*-\s*1\s*;[^)]*\)\s*"
            r"[^;]*heroes\s*\[[^]]*\]\s*=\s*[^;]*heroes\s*\[[^]]*"
            r"\+\s*1\s*\]\s*;\s*--\s*[^;]*numHeroes\s*;\s*[^;]*"
            r"heroes\s*\[[^]]*numHeroes[^]]*\]\s*=\s*-\s*1\s*;"),
        SourceRule(
            "SwapHeroes keeps Dreamcast's nested current/local-owner latch "
            "scope before the former garrison hero's PlaceInMap tail",
            r"if\s*\([^)]*currHeroId\s*==[^)]*->\s*id\s*\)\s*\{\s*"
            r"[^;]*currHeroId\s*=\s*-\s*1\s*;\s*if\s*\(\s*"
            r"gNetLocalGamePos\s*==[^)]*->\s*owner\s*\)\s*\{\s*"
            r"[^;]*drawCursor\s*=\s*0\s*;\s*[^;]*inDialog\s*=\s*0"
            r"\s*;\s*\}\s*\}.*?GetHero\s*\([^;]*->\s*id\s*\)\s*;"
            r".*?\.\s*x\s*=.*?mapX\s*;\s*.*?\.\s*y\s*=.*?mapY\s*;"
            r"\s*.*?\.\s*z\s*=.*?mapZ\s*;\s*.*?PlaceInMap\s*\("),
    ),
    ("town.obj", 0x166B64): (
        SourceRule(
            "set_spells_available keeps Dreamcast's count local inside the "
            "level loop and its base-count, Tower Library, trim-loop and "
            "result-store statement order",
            r"\A\s*memset\s*\(\s*mageGuildSpellCounts\s*,\s*0\s*,\s*"
            r"sizeof\s*\(\s*mageGuildSpellCounts\s*\)\s*\)\s*;\s*"
            r"for\s*\(\s*int\s+level\s*=\s*1\s*;\s*level\s*<=\s*"
            r"field_14\s*;\s*level\+\+\s*\)\s*\{\s*"
            r"int\s+count\s*=\s*gMageGuildBaseSpellCounts\s*\[\s*"
            r"level\s*-\s*1\s*\]\s*;.*?"
            r"HasBuilding\s*\(\s*EXTRA_1_ID\s*,\s*1\s*\).*?"
            r"while\s*\(\s*count\s*>\s*0.*?\)\s*count--\s*;.*?"
            r"mageGuildSpellCounts\s*\[\s*level\s*-\s*1\s*\]\s*=\s*"
            r"count\s*;\s*\}\s*\Z"),
    ),
    ("town.obj", 0x166ED8): (
        SourceRule(
            "destroy_extra_capitol keeps both Dreamcast IsCapitol helper "
            "boundaries",
            r"\bIsCapitol\s*\(\s*\)", 2, 2),
        SourceRule(
            "destroy_extra_capitol keeps Dreamcast's self-Capitol guard, "
            "GetTown/other-Capitol test and three mask updates in order",
            r"\A\s*if\s*\(\s*IsCapitol\s*\(\s*\)\s*&&\s*owner\s*>=\s*0"
            r"\s*\).*?town\s*\*\s*other_town\s*=\s*gpGame\s*->\s*"
            r"GetTown\s*\(\s*town_id\s*\)\s*;\s*if\s*\(\s*"
            r"other_town\s*->\s*IsCapitol\s*\(\s*\)\s*\)\s*\{\s*"
            r"built\s*&=\s*~\s*bitNumber\s*\[\s*HALL_CAPITOL_ID\s*\]"
            r"\s*;\s*built\s*\|=\s*bitNumber\s*\[\s*HALL_CITY_ID\s*\]"
            r"\s*;\s*active\s*&=\s*~\s*bitNumber\s*\[\s*"
            r"HALL_CAPITOL_ID\s*\]\s*;"),
        SourceRule(
            "destroy_extra_capitol keeps Dreamcast's NewfullMap::cell "
            "boundary immediately before ConvertObject",
            r"NewmapCell\s*\*\s*cell\s*=\s*gpGame\s*->\s*worldMap\s*\.\s*"
            r"cell\s*\(\s*mapX\s*,\s*mapY\s*,\s*mapZ\s*\)\s*;\s*"
            r"gpGame\s*->\s*ConvertObject\s*\(\s*cell\s*\)\s*;"),
    ),
    ("town.obj", 0x166FC8): (
        SourceRule(
            "BuildBuilding keeps raw NB11's built local and the recovered "
            "IsCastle, IsCapitol and create_building opening order",
            r"\A\s*type_building_id\s+built\s*;\s*"
            r"unsigned\s+char\s+had_fort\s*=\s*IsCastle\s*\(\s*\)\s*;"
            r"\s*unsigned\s+char\s+had_capitol\s*=\s*IsCapitol\s*\(\s*"
            r"\)\s*;\s*built\s*=\s*create_building\s*\("),
        SourceRule(
            "BuildBuilding keeps Dreamcast's one update_full_building_mask "
            "helper boundary",
            r"\bupdate_full_building_mask\s*\(\s*\)\s*;", 1, 1),
        SourceRule(
            "BuildBuilding keeps both Dreamcast set_spells_available helper "
            "boundaries",
            r"\bset_spells_available\s*\(\s*\)\s*;", 2, 2),
        SourceRule(
            "BuildBuilding keeps Dreamcast's later IsCapitol then IsCastle "
            "change test against the two opening snapshots",
            r"\bGiveSpells\s*\(\s*0\s*\)\s*;.*?"
            r"if\s*\(\s*\(\s*IsCapitol\s*\(\s*\)\s*&&\s*!\s*"
            r"had_capitol\s*\)\s*\|\|\s*\(\s*IsCastle\s*\(\s*\)\s*"
            r"&&\s*!\s*had_fort\s*\)\s*\)"),
    ),
    ("town.obj", 0x168494): (
        SourceRule(
            "update_full_building_mask keeps Dreamcast's active assignment "
            "and HasBuilding-driven included-building sweep",
            r"\A\s*active\s*=\s*built\s*;\s*for\s*\(\s*int\s+i\s*=\s*0"
            r"\s*;\s*i\s*<\s*MAX_BUILDING_TYPE\s*;\s*i\+\+\s*\)\s*"
            r"\{\s*if\s*\(\s*HasBuilding\s*\(\s*i\s*,\s*0\s*\)\s*"
            r"\)\s*active\s*\|=\s*included_buildings\s*\[\s*type\s*\]"
            r"\s*\[\s*i\s*\]\s*;\s*\}\s*\Z"),
    ),
    ("victorylossconditions.obj", 0x190124): (
        SourceRule(
            "CheckForGrailBuildingWin keeps the recovered grail_town_loc "
            "and any_town_loc identities in Dreamcast constructor order",
            r"type_point\s+any_town_loc\s*\(\s*-1\s*,\s*-1\s*,\s*-1"
            r"\s*\)\s*;\s*type_point\s+grail_town_loc\s*\(\s*TownX\s*,"
            r"\s*TownY\s*,\s*TownZ\s*\)\s*;"),
        SourceRule(
            "CheckForGrailBuildingWin keeps the game::OnSameTeam boundary",
            r"gpGame\s*->\s*OnSameTeam\s*\(\s*player\s*,\s*"
            r"gNetLocalGamePos\s*\)"),
        SourceRule(
            "CheckForGrailBuildingWin keeps this_town_loc and both "
            "Dreamcast const-reference equality expressions",
            r"type_point\s+this_town_loc\s*\(\s*thisTown\s*->\s*mapX\s*,"
            r"\s*thisTown\s*->\s*mapY\s*,\s*thisTown\s*->\s*mapZ\s*\)"
            r"\s*;\s*if\s*\(\s*this_town_loc\s*==\s*grail_town_loc\s*"
            r"\|\|\s*grail_town_loc\s*==\s*any_town_loc\s*\)"),
        SourceRule(
            "CheckForGrailBuildingWin keeps the active-mask HasBuilding "
            "boundary inside the matching-location arm",
            r"if\s*\(\s*this_town_loc\s*==.*?\)\s*\{\s*if\s*\(\s*"
            r"thisTown\s*->\s*HasBuilding\s*\(\s*HOLY_GRAIL_ID\s*,\s*1"
            r"\s*\)\s*\)"),
    ),
    ("spells.obj", 0x153B60): (
        SourceRule(
            "AreaEffect keeps Dreamcast's casting_hero, multiple_targets, "
            "targets and damage locals in CodeView declaration order and "
            "under their recovered names",
            r"\A\s*hero\s*\*\s*casting_hero\s*;\s*unsigned\s+char\s+"
            r"multiple_targets\s*;.*?std\s*::\s*vector\s*<\s*army\s*\*\s*>"
            r"\s+targets\s*;.*?\blong\s+damage\s*;"),
        SourceRule(
            "AreaEffect keeps Dreamcast's initial SpellEffect, targets "
            "vector construction and mark_area_effect statement order",
            r"\bSpellEffect\s*\(.*?\)\s*;\s*std\s*::\s*vector\s*<\s*army"
            r"\s*\*\s*>\s+targets\s*;.*?\bmark_area_effect\s*\(.*?"
            r"\btargets\s*\)\s*;"),
        SourceRule(
            "AreaEffect keeps both Dreamcast ComputeSpellDamage statements "
            "bound to its recovered casting_hero local",
            r"\bComputeSpellDamage\s*\([^;]*?\bcasting_hero\b", 2, 2),
        SourceRule(
            "AreaEffect keeps Dreamcast's multiple_targets initialization, "
            "second-victim assignment and final branch order",
            r"\bmultiple_targets\s*=\s*0\s*;.*?"
            r"if\s*\(\s*!\s*victim\s*\)\s*victim\s*=\s*target\s*;\s*"
            r"else\s*multiple_targets\s*=\s*1\s*;.*?"
            r"if\s*\(\s*victim\s*\)\s*\{\s*"
            r"if\s*\(\s*multiple_targets\s*\)"),
        SourceRule(
            "AreaEffect keeps Dreamcast's failed-target effected clear "
            "before the successful-target damage statement",
            r"\beffected\s*\[\s*target\s*->\s*combatSide\s*\]\s*"
            r"\[\s*target\s*->\s*bitIndex\s*\]\s*=\s*0\s*;.*?"
            r"\bdamage\s*=\s*ComputeSpellDamage\s*\("),
    ),
    ("soundmgr.obj", 0x14B528): (
        SourceRule(
            "MemorySample keeps Dreamcast lines 815/817's next-slot advance "
            "and one-statement wrapped first/next assignment",
            r"\bslot\s*=\s*range\s*->\s*next\s*\+\+\s*;\s*"
            r"if\s*\(\s*range\s*->\s*next\s*>=\s*range\s*->\s*last\s*"
            r"\)\s*\{\s*slot\s*=\s*range\s*->\s*next\s*=\s*"
            r"range\s*->\s*first\s*;"),
        SourceRule(
            "MemorySample keeps Dreamcast's StopSample helper boundary on "
            "the selected channel instead of expanding its body",
            r"\bStopSample\s*\(\s*sampleHandles\s*\[\s*slot\s*\]\s*"
            r"\)\s*;", 1, 1),
        SourceRule(
            "MemorySample keeps Dreamcast's StopSample, ConvertVolume, "
            "volume application, sample start, handle store and return "
            "statement order through Complete's Miles adapters",
            r"\bStopSample\s*\(.*?\bAIL_set_sample_volume\s*\(\s*"
            r"handle\s*,\s*ConvertVolume\s*\(.*?\)\s*\)\s*;.*?"
            r"\bAIL_start_sample\s*\(\s*handle\s*\)\s*;.*?"
            r"\bsPtr\s*->\s*field_1c\s*=\s*handle\s*;.*?"
            r"\breturn\s+handle\s*;"),
        SourceRule(
            "MemorySample keeps exactly one Dreamcast ConvertVolume helper "
            "call in the volume-selection statement group",
            r"\bConvertVolume\s*\(", 1, 1),
        SourceRule(
            "MemorySample keeps Complete's retail-corroborated named "
            "service_sounds boundary after releasing its sample section",
            r"\bLeaveCriticalSection\s*\(\s*&\s*section_sound_call\s*\)"
            r"\s*;\s*gpSoundManager\s*->\s*service_sounds\s*\(\s*\)"
            r"\s*;\s*return\s+handle\s*;", 1, 1),
    ),
    ("soundmgr.obj", 0x14B780): (
        SourceRule(
            "launch_sample keeps Complete's retail-corroborated "
            "MemorySample then service_sounds helper order before worker "
            "launch",
            r"gpSoundManager\s*->\s*MemorySample\s*\(.*?\)\s*;\s*"
            r"gpSoundManager\s*->\s*service_sounds\s*\(\s*\)\s*;\s*"
            r"if\s*\(\s*!\s*bShutDownDone\s*\)"),
        SourceRule(
            "launch_sample keeps exactly one named Complete "
            "service_sounds helper call",
            r"\bservice_sounds\s*\(", 1, 1),
    ),
    ("palette.obj", 0x10AA98): (
        SourceRule(
            "TPalette16::Cycle keeps Dreamcast's positive-step loop, saved "
            "begin endpoint, left memmove and end writeback sequence",
            r"if\s*\(\s*step\s*>\s*0\s*\)\s*\{\s*"
            r"for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*step\s*;\s*"
            r"\+\+\s*i\s*\)\s*\{\s*"
            r"unsigned\s+short\s+saved\s*=\s*data\s*\[\s*begin\s*\]"
            r"\s*;\s*memmove\s*\(\s*&\s*data\s*\[\s*begin\s*\]\s*,"
            r"\s*&\s*data\s*\[\s*begin\s*\+\s*1\s*\]\s*,\s*"
            r"\(\s*end\s*-\s*begin\s*\)\s*\*\s*sizeof\s*\(\s*"
            r"data\s*\[\s*0\s*\]\s*\)\s*\)\s*;\s*"
            r"data\s*\[\s*end\s*\]\s*=\s*saved\s*;", 1, 1),
        SourceRule(
            "TPalette16::Cycle keeps Dreamcast's negative-step loop, saved "
            "end endpoint, right memmove and begin writeback sequence",
            r"else\s*\{\s*for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*"
            r"-\s*step\s*;\s*\+\+\s*i\s*\)\s*\{\s*"
            r"unsigned\s+short\s+saved\s*=\s*data\s*\[\s*end\s*\]"
            r"\s*;\s*memmove\s*\(\s*&\s*data\s*\[\s*begin\s*\+\s*"
            r"1\s*\]\s*,\s*&\s*data\s*\[\s*begin\s*\]\s*,\s*"
            r"\(\s*end\s*-\s*begin\s*\)\s*\*\s*sizeof\s*\(\s*"
            r"data\s*\[\s*0\s*\]\s*\)\s*\)\s*;\s*"
            r"data\s*\[\s*begin\s*\]\s*=\s*saved\s*;", 1, 1),
    ),
    ("palette.obj", 0x10B7AC): (
        SourceRule(
            "TPalette16::Gray keeps Dreamcast's three const unsigned "
            "normalization locals and their red, green, blue source-row "
            "order",
            r"\A\s*const\s+unsigned\s+int\s+red_norm\s*=\s*"
            r"std\s*::\s*numeric_limits\s*<\s*int\s*>\s*::\s*max\s*"
            r"\(\s*\)\s*/\s*red_mask\s*;\s*"
            r"const\s+unsigned\s+int\s+green_norm\s*=\s*"
            r"std\s*::\s*numeric_limits\s*<\s*int\s*>\s*::\s*max\s*"
            r"\(\s*\)\s*/\s*green_mask\s*;\s*"
            r"const\s+unsigned\s+int\s+blue_norm\s*=\s*"
            r"std\s*::\s*numeric_limits\s*<\s*int\s*>\s*::\s*max\s*"
            r"\(\s*\)\s*/\s*blue_mask\s*;", 1, 1),
        SourceRule(
            "TPalette16::Gray keeps Dreamcast's entry-10 loop, three "
            "channel statements and nested max(max(red, green), blue) "
            "boundary",
            r"for\s*\(\s*int\s+i\s*=\s*10\s*;\s*i\s*<\s*256\s*;\s*"
            r"\+\+\s*i\s*\)\s*\{\s*"
            r"unsigned\s+int\s+red\s*=\s*\(\s*data\s*\[\s*i\s*\]"
            r"\s*&\s*red_mask\s*\)\s*\*\s*red_norm\s*;\s*"
            r"unsigned\s+int\s+green\s*=\s*\(\s*data\s*\[\s*i\s*\]"
            r"\s*&\s*green_mask\s*\)\s*\*\s*green_norm\s*;\s*"
            r"unsigned\s+int\s+blue\s*=\s*\(\s*data\s*\[\s*i\s*\]"
            r"\s*&\s*blue_mask\s*\)\s*\*\s*blue_norm\s*;\s*"
            r"unsigned\s+int\s+gray\s*=\s*max\s*\(\s*max\s*\(\s*"
            r"red\s*,\s*green\s*\)\s*,\s*blue\s*\)\s*;", 1, 1),
        SourceRule(
            "TPalette16::Gray keeps Dreamcast's single packed write with "
            "red, green and blue normalization/mask pairings",
            r"data\s*\[\s*i\s*\]\s*=\s*static_cast\s*<\s*"
            r"unsigned\s+short\s*>\s*\(\s*"
            r"\(\s*\(\s*gray\s*/\s*red_norm\s*\)\s*&\s*red_mask\s*\)"
            r"\s*\|\s*"
            r"\(\s*\(\s*gray\s*/\s*green_norm\s*\)\s*&\s*"
            r"green_mask\s*\)\s*\|\s*"
            r"\(\s*\(\s*gray\s*/\s*blue_norm\s*\)\s*&\s*"
            r"blue_mask\s*\)\s*\)\s*;", 1, 1),
    ),
    ("resourcemanager.obj", 0x121EC8): (
        SourceRule(
            "GetPalette24 keeps Dreamcast's char[24] header and TRGBA[256] "
            "rgba locals in CodeView declaration order and under their "
            "recovered names",
            r"\bchar\s+header\s*\[\s*24\s*\]\s*;\s*"
            r"TRGBA\s+rgba\s*\[\s*256\s*\]\s*;"),
        SourceRule(
            "GetPalette24 keeps the retail-corroborated ordinary/archive "
            "copies of Dreamcast's header-then-rgba read sequence",
            r"streamInterface\s*->\s*Read\s*\(\s*header\s*,\s*"
            r"sizeof\s*\(\s*header\s*\)\s*\)\s*;\s*"
            r"streamInterface\s*->\s*Read\s*\(\s*rgba\s*,\s*"
            r"sizeof\s*\(\s*rgba\s*\)\s*\)\s*;", 2, 2),
        SourceRule(
            "GetPalette24 keeps both retail paths on Dreamcast's direct "
            "TPalette24 construction followed by optional AdjustHSV shape",
            r"result\s*=\s*new\s+TPalette24\s*\(\s*rgba\s*\)\s*;\s*"
            r"if\s*\(\s*gGraphicsSaturated\s*\)\s*"
            r"result\s*->\s*AdjustHSV\s*\(", 2, 2),
    ),
    ("recruit.obj", 0x119DCC): (
        SourceRule(
            "recruitUnit::Update keeps Dreamcast's root message constructor "
            "before the slot guard and named UpdateCost boundary",
            r"\A\s*message\s+msg\s*;\s*if\s*\(\s*slot\s*==\s*-\s*1\s*"
            r"\)\s*slot\s*=\s*selectedPosition\s*;\s*UpdateCost\s*"
            r"\(\s*\)\s*;"),
        SourceRule(
            "recruitUnit::Update keeps Dreamcast line 521's one-statement "
            "TTextResource::operator[], GetArmyName and sprintf group",
            r"sprintf\s*\(\s*gText\s*,[^,]*,\s*"
            r"\(\s*\*\s*gpGeneralText\s*\)\s*\[\s*"
            r"GENERAL_TEXT_RECRUIT_TITLE\s*\]\s*,\s*GetArmyName\s*\(\s*"
            r"monsterType\s*,\s*2\s*\)\s*\)\s*;", 1, 1),
        SourceRule(
            "recruitUnit::Update may not flatten Dreamcast's GetArmyName "
            "boundary back into a creature-trait plural-name read",
            r"akCreatureTypeTraits\s*\[\s*monsterType\s*\]\s*\.\s*"
            r"m_plural_name", 0, 0),
        SourceRule(
            "recruitUnit::Update keeps raw NB11's long maxGold local and "
            "the alternative-resource selection order",
            r"\blong\s+maxGold\s*=\s*gpCurrentPlayer\s*->\s*resources\s*"
            r"\[\s*6\s*\]\s*/\s*goldPerTroop\s*;\s*if\s*\(\s*"
            r"altResource\s*!=\s*-\s*1\s*\)\s*\{.*?maxAvail\s*=\s*"
            r"maxGold\s*<.*?\?.*?\}\s*else\s*\{\s*maxAvail\s*=\s*"
            r"maxGold\s*;\s*\}"),
    ),
    ("recruit.obj", 0x11AC7C): (
        SourceRule(
            "UpdateCost keeps raw NB11's sole resCost local and the "
            "Dreamcast GetMonsterCost helper boundary before gold and scan",
            r"\A\s*int\s+resCost\s*\[\s*7\s*\]\s*;\s*GetMonsterCost\s*"
            r"\(\s*monsterType\s*,\s*resCost\s*\)\s*;\s*goldPerTroop\s*"
            r"=\s*resCost\s*\[\s*6\s*\]\s*;\s*int\s+i\s*;\s*for\s*"
            r"\(\s*i\s*=\s*0\s*;\s*i\s*<\s*6\s*;\s*i\+\+\s*\)"),
        SourceRule(
            "UpdateCost may not replace Dreamcast's GetMonsterCost helper "
            "with a duplicated record-table memcpy or index loop",
            r"\b(?:memcpy|gCreatureRecords)\b", 0, 0),
    ),
    ("resourcedisplay.obj", 0x120C54): (
        SourceRule(
            "TResourceDisplay constructor keeps Dreamcast's paired size "
            "arms with initialize before the matching resource background "
            "construction",
            r"if\s*\(\s*(?:isSmall|is_small)\s*\)\s*\{\s*"
            r"initialize\s*\(\s*7\s*,\s*0x23f\s*,\s*0x2e2\s*,\s*"
            r"0x16\s*,\s*parent\s*\)\s*;\s*resourceBackground\s*=\s*"
            r"new\s+bitmapBorder\s*\([^;]*0x2e2[^;]*\)\s*;\s*"
            r"\}\s*else\s*\{\s*initialize\s*\(\s*3\s*,\s*0x23f\s*,"
            r"\s*0x31a\s*,\s*0x16\s*,\s*parent\s*\)\s*;\s*"
            r"resourceBackground\s*=\s*new\s+bitmapBorder\s*\([^;]*"
            r"0x31a[^;]*\)\s*;\s*\}", 1, 1),
        SourceRule(
            "TResourceDisplay constructor keeps the palette statement before "
            "one seven-resource text/add/border/add loop",
            r"resourceBackground\s*->\s*SetPlayerPaletteColors\s*\(\s*"
            r"gpGame\s*->\s*GetLocalPlayerGamePos\s*\(\s*\)\s*\)\s*;"
            r".*?for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*"
            r"NUM_RESOURCES\s*;\s*\+\+i\s*\)\s*\{\s*"
            r"resourceWidgets\s*\[\s*i\s*\]\s*=\s*new\s+textWidget"
            r"\s*\([^;]*\)\s*;\s*AddWidget\s*\(\s*resourceWidgets"
            r"\s*\[\s*i\s*\]\s*,\s*-\s*1\s*\)\s*;\s*"
            r"resourceBorders\s*\[\s*i\s*\]\s*=\s*new\s+border"
            r"\s*\([^;]*\)\s*;\s*AddWidget\s*\(\s*resourceBorders"
            r"\s*\[\s*i\s*\]\s*,\s*-\s*1\s*\)\s*;[^}]*\}",
            1, 1),
        SourceRule(
            "TResourceDisplay constructor keeps the final flag-selected "
            "status-widget constructions before one shared AddWidget",
            r"if\s*\([^{};]*(?:isSmall|is_small)[^{};]*\)\s*\{\s*"
            r"statusWidget\s*=\s*new\s+textWidget\s*\([^;]*\)\s*;\s*"
            r"\}\s*else\s*\{\s*statusWidget\s*=\s*new\s+textWidget"
            r"\s*\([^;]*\)\s*;\s*\}\s*AddWidget\s*\(\s*statusWidget"
            r"\s*,\s*-\s*1\s*\)\s*;", 1, 1),
    ),
    ("philai.obj", 0x10FEB8): (
        SourceRule(
            "value_of_experience keeps Dreamcast's no-argument const hero "
            "accessor at line 1718",
            r"\bint\s+increment\s*=\s*current_hero\s*->\s*"
            r"GetExperienceIncrement\s*\(\s*\)\s*;"),
        SourceRule(
            "value_of_experience keeps the line-1719 army conversion as a "
            "separate statement through the attested const-reference before "
            "the line-1721 return",
            r"\bfloat\s+army_value\s*=\s*float\s*\(\s*current_army\s*\.\s*"
            r"get_AI_value\s*\(\s*\)\s*\)\s*;\s*return\b"),
        SourceRule(
            "value_of_experience keeps the retail-exact line-1721 float "
            "gold/add/divide expression",
            r"return\s*\(\s*float\s*\(\s*gHeroGoldCost\s*\)\s*\+\s*"
            r"army_value\s*\)\s*/\s*float\s*\(\s*increment\s*\*\s*40\s*"
            r"\)\s*;"),
    ),
    ("philai.obj", 0x10FEF4): (
        SourceRule(
            "AI_set_hero_bonuses keeps Dreamcast's sole caster local and "
            "the value_of_experience statement before spell valuation",
            r"\btype_spellvalue\s+caster\s*\(\s*our_hero\s*\)\s*;\s*"
            r"our_hero\s*->\s*turnExperienceToRVRatio\s*=\s*"
            r"value_of_experience\s*\(\s*our_hero\s*,\s*"
            r"our_hero\s*->\s*army\s*\)\s*;\s*long\s+base_value\s*=\s*"
            r"caster\s*\.\s*get_best_spell_value\s*\("),
        SourceRule(
            "AI_set_hero_bonuses keeps both source-visible max wrappers",
            r"(?<![_\w])max\s*\(", 2, 2),
        SourceRule(
            "AI_set_hero_bonuses calls set_value_of_power exactly once",
            r"\bset_value_of_power\s*\(", 1, 1),
        SourceRule(
            "AI_set_hero_bonuses calls set_value_of_duration exactly once",
            r"\bset_value_of_duration\s*\(", 1, 1),
        SourceRule(
            "AI_set_hero_bonuses calls set_value_of_knowledge exactly once",
            r"\bset_value_of_knowledge\s*\(", 1, 1),
        SourceRule(
            "AI_set_hero_bonuses calls set_value_of_well in both source arms",
            r"\bset_value_of_well\s*\(", 2, 2),
        SourceRule(
            "AI_set_hero_bonuses calls set_value_of_spring in both source arms",
            r"\bset_value_of_spring\s*\(", 2, 2),
        SourceRule(
            "AI_set_hero_bonuses may not flatten the five attested setters "
            "back into direct field assignments",
            r"our_hero\s*->\s*value_of_(?:power|duration|knowledge|well|spring)"
            r"\s*=", 0, 0),
        SourceRule(
            "AI_set_hero_bonuses keeps the line-1735/1737, 1739/1740, "
            "and 1742/1744 calculation/setter statement splits",
            r"\blong\s+value\s*=\s*max\s*\(\s*caster\."
            r"get_value_of_increase\s*\([^;]*;\s*our_hero\s*->\s*"
            r"set_value_of_power\s*\(\s*value\s*\)\s*;\s*value\s*=\s*"
            r"caster\.get_value_of_increase\s*\([^;]*;\s*our_hero\s*->\s*"
            r"set_value_of_duration\s*\(\s*value\s*\)\s*;\s*value\s*=\s*"
            r"max\s*\(\s*caster\.get_value_of_increase\s*\([^;]*;\s*"
            r"our_hero\s*->\s*set_value_of_knowledge\s*\(\s*value\s*\)"
            r"\s*;"),
    ),
    ("philai.obj", 0x112510): (
        SourceRule(
            "ValueOfScroll keeps the customized/non-customized spell "
            "assignment arms before the semantic SpellID constructor",
            r"\bSpellID\s+spell\s*;.*?if\s*\(\s*cell\s*->\s*"
            r"IsCustomized\s*\(\s*\)\s*\)\s*\{.*?spell\s*=\s*"
            r"cell\s*->\s*extraInfo\s*&\s*0xff\s*;.*?\}\s*else\s*"
            r"spell\s*=\s*cell\s*->\s*extraInfo\s*;\s*"
            r"type_artifact\s+artifact\s*\(\s*spell\s*\)\s*;"),
        SourceRule(
            "ValueOfScroll may not flatten type_artifact(SpellID) into "
            "manual artifact record writes",
            r"\bartifact\s*\.\s*(?:artifactId|extra)\s*=", 0, 0),
    ),
    ("philai.obj", 0x112F6C): (
        SourceRule(
            "ValueOfTreasure keeps Dreamcast lines 3315-3318 as a full "
            "first-pair assignment into an otherwise uninitialized value",
            r"\bint\s+value\s*;\s*if\s*\(\s*experience_part\s*>\s*"
            r"gold_part\s*\)\s*value\s*=\s*experience_part\s*;\s*"
            r"else\s*value\s*=\s*gold_part\s*;"),
    ),
    ("adventureoptionswindow.obj", 0x5204): (
        SourceRule(
            "TAdventureOptionsWindow::WindowHandler keeps explicit close "
            "state before PollSound (Dreamcast lines 153 and 237-242)",
            r"\b(?:bool|int|unsigned\s+char)\s+closeDialog\s*=\s*"
            r"(?:false|0)\s*;\s*PollSound\s*\(\s*\)\s*;"),
        SourceRule(
            "TAdventureOptionsWindow::WindowHandler keeps the mouse "
            "coordinates as direct findWidget arguments in their one "
            "Dreamcast line-211 statement",
            r"\bfindWidget\s*\(\s*msg\s*->\s*mouseX\s*,\s*"
            r"msg\s*->\s*mouseY\s*\)"),
        SourceRule(
            "TAdventureOptionsWindow::WindowHandler sets close state once "
            "for the selected option",
            r"\bcloseDialog\s*=\s*(?:true|1)\s*;", 1, 1),
        SourceRule(
            "TAdventureOptionsWindow::WindowHandler keeps widget dispatch "
            "before the mouse else-if arm (Dreamcast lines 169 then 211)",
            r"if\s*\(\s*msg\s*->\s*id\s*==\s*MESSAGE_WIDGET\s*\)\s*\{"
            r".*?closeDialog\s*=\s*(?:true|1)\s*;.*?\}\s*else\s+if\s*"
            r"\(\s*msg\s*->\s*id\s*==\s*MESSAGE_MOUSE_MOVE\s*\)"),
        SourceRule(
            "TAdventureOptionsWindow::WindowHandler consumes close state in "
            "one shared tail after the mouse arm (Dreamcast lines 237-242)",
            r"DrawWindow\s*\([^;]*\)\s*;.*?\}\s*\}\s*if\s*\(\s*"
            r"closeDialog\s*\)\s*\{\s*msg\s*->\s*id\s*=\s*"
            r"MESSAGE_WIDGET\s*;\s*gpWindowManager\s*->\s*dialogReturn\s*"
            r"=\s*msg\s*->\s*codeY\s*;.*?return\s+"
            r"MESSAGE_DISPATCH_FORWARD\s*;\s*\}"),
        SourceRule(
            "TAdventureOptionsWindow::WindowHandler keeps one shared "
            "forward return instead of the 99.9367% duplicated-tail plateau",
            r"return\s+MESSAGE_DISPATCH_FORWARD\s*;", 1, 1),
        SourceRule(
            "TAdventureOptionsWindow::WindowHandler keeps one shared consume "
            "return instead of the 99.9367% early-return plateau",
            r"return\s+MESSAGE_DISPATCH_CONSUME\s*;", 1, 1),
        SourceRule(
            "TAdventureOptionsWindow uses its distinct attested options-help "
            "table in both help paths",
            r"\bgAdventureOptionsHelp\s*\[", 2, 2),
    ),
    ("armygrp.obj", 0x4E428): (
        SourceRule(
            "TSplitWindow::WindowHandler keeps explicit close/update state "
            "initialized before the base handler call (Dreamcast line 230)",
            r"\b(?:bool|int|long|unsigned\s+char)\s+closeDialog\s*=\s*"
            r"(?:false|0)\s*(?:,\s*updateArmy\s*=\s*(?:false|0)|;\s*"
            r"(?:bool|int|long|unsigned\s+char)\s+updateArmy\s*=\s*"
            r"(?:false|0))\s*;[^{}]*\bCAdvPopup\s*::\s*WindowHandler\s*\("),
        SourceRule(
            "TSplitWindow::WindowHandler keeps the source-entry edit arm "
            "before the destination-entry arm (Dreamcast lines 274-289)",
            r"case\s+SPLIT_WIDGET_SOURCE_ENTRY\s*:.*?"
            r"case\s+SPLIT_WIDGET_DESTINATION_ENTRY\s*:"),
        SourceRule(
            "TSplitWindow::WindowHandler keeps one shared slider update "
            "after the edit switch (Dreamcast line 291)",
            r"case\s+SPLIT_WIDGET_SOURCE_ENTRY\s*:.*?break\s*;\s*"
            r"case\s+SPLIT_WIDGET_DESTINATION_ENTRY\s*:.*?break\s*;\s*"
            r"\}\s*splitSlider\s*->\s*SetState\s*\(\s*"
            r"destinationTroops\s*\)\s*;"),
        SourceRule(
            "TSplitWindow::WindowHandler has exactly one source-authored "
            "slider update",
            r"\bsplitSlider\s*->\s*SetState\s*\(\s*destinationTroops\s*\)"
            r"\s*;", 1, 1),
        SourceRule(
            "TSplitWindow::WindowHandler sets update state once after the "
            "edit switch (Dreamcast line 292)",
            r"\bupdateArmy\s*=\s*(?:true|1)\s*;", 1, 1),
        SourceRule(
            "TSplitWindow::WindowHandler keeps the low close choices, then "
            "accept, then one shared close-state assignment (Dreamcast "
            "lines 297-306)",
            r"case\s+DIALOG_RETURN_SPLIT_CLOSE\s*:\s*"
            r"case\s+DIALOG_RETURN_SPLIT_CANCEL\s*:.*?"
            r"dialogReturn\s*=\s*msg\s*->\s*codeY\s*;\s*break\s*;\s*"
            r"case\s+DIALOG_RETURN_SPLIT_ACCEPT\s*:.*?"
            r"dialogReturn\s*=\s*DIALOG_RETURN_SPLIT_ACCEPT\s*;\s*"
            r"break\s*;\s*default\s*:.*?\}\s*"
            r"closeDialog\s*=\s*(?:true|1)\s*;"),
        SourceRule(
            "TSplitWindow::WindowHandler sets close state exactly once",
            r"\bcloseDialog\s*=\s*(?:true|1)\s*;", 1, 1),
        SourceRule(
            "TSplitWindow::WindowHandler consumes close state before update "
            "state in the shared function tail (Dreamcast lines 322-330)",
            r"if\s*\(\s*closeDialog(?:\s*==\s*(?:true|1))?\s*\)\s*\{"
            r".*?msg\s*->\s*codeY\s*=\s*widget\s*::\s*WIDGET_END_DIALOG"
            r"\s*;.*?msg\s*->\s*codeX\s*=\s*widget\s*::\s*"
            r"WIDGET_END_DIALOG\s*;.*?return\s+MESSAGE_DISPATCH_FORWARD\s*;"
            r"\s*\}\s*if\s*\(\s*updateArmy\s*\)\s*"
            r"UpdateSplitArmy\s*\(\s*1\s*\)\s*;"),
        SourceRule(
            "TSplitWindow::WindowHandler keeps one source-authored end-dialog "
            "tail instead of the duplicated 99.917% local maximum",
            r"msg\s*->\s*codeY\s*=\s*widget\s*::\s*WIDGET_END_DIALOG\s*;",
            1, 1),
        SourceRule(
            "TSplitWindow::WindowHandler keeps one source-authored forward "
            "return instead of the duplicated 99.917% local maximum",
            r"return\s+MESSAGE_DISPATCH_FORWARD\s*;", 1, 1),
        SourceRule(
            "TSplitWindow::WindowHandler keeps one shared UpdateSplitArmy "
            "consumer",
            r"\bUpdateSplitArmy\s*\(\s*1\s*\)\s*;", 1, 1),
        SourceRule(
            "TSplitWindow::WindowHandler keeps the positive hover-change "
            "scope, SetRolloverText boundary, and mouse-arm return "
            "(Dreamcast lines 313-318)",
            r"if\s*\(\s*msg\s*->\s*codeY\s*!=\s*gpWindowManager\s*->\s*"
            r"lastHover\s*\)\s*\{\s*gpWindowManager\s*->\s*lastHover\s*=\s*"
            r"msg\s*->\s*codeY\s*;\s*SetRolloverText\s*\(\s*msg\s*->\s*"
            r"codeY\s*\)\s*;\s*\}\s*return\s+"
            r"MESSAGE_DISPATCH_CONSUME\s*;"),
    ),
    ("armygrp.obj", 0x4F078): (
        SourceRule(
            "armyGroup::GetMorale keeps Dreamcast's HasSomeUndead member "
            "boundary after Complete's alignment adjustment and before "
            "the Angel membership tests",
            r"morale\s*\+=\s*2\s*-\s*numAlignments\s*;\s*"
            r"if\s*\(\s*HasSomeUndead\s*\(\s*\)\s*\)\s*"
            r"morale--\s*;\s*if\s*\(\s*IsMember\s*\("),
    ),
    ("armygrp.obj", 0x4F708): (
        SourceRule(
            "armyGroup::get_morale_description keeps Dreamcast's "
            "HasSomeUndead member boundary and undead-text statement before "
            "the Angel membership group",
            r"if\s*\(\s*HasSomeUndead\s*\(\s*\)\s*\)\s*"
            r"result\s*\.\s*append\s*\(\s*gUndeadMoraleText\s*\)\s*;"
            r"\s*TCreatureType\s+angelType\s*=\s*CREATURE_NONE\s*;\s*"
            r"if\s*\(\s*IsMember\s*\("),
    ),
    ("armygrp.obj", 0x4DBB8): (
        SourceRule(
            "TSplitWindow keeps exactly thirteen direct Widgets.push_back "
            "statements",
            r"\bWidgets\s*\.\s*push_back\s*\(", 13, 13),
        SourceRule(
            "TSplitWindow keeps TTextResource::operator[] nested in sprintf",
            r"\bsprintf\s*\(\s*gText\s*,\s*\(\s*\*\s*gpGeneralText\s*\)"
            r"\s*\[\s*GENERAL_TEXT_SPLIT_CREATURE_ROLLOVER\s*\]"),
        SourceRule(
            "TSplitWindow keeps sourceEntry construction then push_back",
            r"sourceEntry\s*=\s*new\s+textEntryWidget\s*\([^;]*\)\s*;\s*"
            r"Widgets\s*\.\s*push_back\s*\(\s*sourceEntry\s*\)\s*;"),
        SourceRule(
            "TSplitWindow keeps destinationEntry construction then push_back",
            r"destinationEntry\s*=\s*new\s+textEntryWidget\s*\([^;]*\)"
            r"\s*;\s*Widgets\s*\.\s*push_back\s*\(\s*destinationEntry\s*"
            r"\)\s*;"),
        SourceRule(
            "TSplitWindow keeps splitSlider construction then push_back",
            r"splitSlider\s*=\s*new\s+slider\s*\([^;]*slider\s*::\s*"
            r"BROWN[^;]*\)\s*;\s*"
            r"Widgets\s*\.\s*push_back\s*\(\s*splitSlider\s*\)\s*;"),
        SourceRule(
            "TSplitWindow status text keeps the Dreamcast-proven null text "
            "argument",
            r"new\s+textWidget\s*\(\s*8\s*,\s*312\s*,\s*282\s*,\s*17\s*,"
            r"\s*(?:0|NULL)\s*,"),
        SourceRule(
            "TSplitWindow keeps both button constructions in direct "
            "push_back statements",
            r"Widgets\s*\.\s*push_back\s*\(\s*new\s+button\s*\([^;]*"
            r"DIALOG_RETURN_SPLIT_ACCEPT[^;]*\)\s*\)\s*;\s*"
            r"Widgets\s*\.\s*push_back\s*\(\s*new\s+button\s*\([^;]*"
            r"0x7801[^;]*\)\s*\)\s*;"),
    ),
    ("army.obj", 0x46BEC): (
        SourceRule(
            "ResetHitByCreature precedes the behind zeroing "
            "(Dreamcast source lines 2048 then 2052)",
            r"\bResetHitByCreature\s*\([^;]*\)\s*;\s*"
            r"behind\s*=\s*(?:0|NULL)\s*;"),
        SourceRule("three source-visible CSprite::IsValidSeq tests",
                   r"\bIsValidSeq\s*\(", 3),
        SourceRule("all three effect marks use get_owning_side",
                   r"\bMarkCreatureEffect\s*\(\s*"
                   r"(?:\w+\s*->\s*)?get_owning_side\s*\(", 3),
    ),
    ("army.obj", 0x475EC): (
        SourceRule("CheckLuck calls SRandom", r"\bSRandom\s*\("),
        SourceRule("CheckLuck preserves the min wrapper", r"\bmin\s*\("),
        SourceRule("CheckLuck uses TTextResource::operator[]",
                   r"\(\s*\*\s*gpGeneralText\s*\)\s*\["),
    ),
    ("army.obj", 0x4868C): (
        SourceRule(
            "ComputeAttackerDamageBonuses keeps Dreamcast's ballista-arm "
            "army::GetName helper boundary before formatting",
            r"\bconst\s+char\s*\*\s*creature_name\s*;\s*"
            r"creature_name\s*=\s*GetName\s*\(\s*\)\s*;.*?"
            r"format_string\s*\([^;]*\bcreature_name\b"),
        SourceRule(
            "ComputeAttackerDamageBonuses may not force Dreamcast's "
            "GetName helper out of line with an inline-depth fence",
            r"#\s*pragma\s+inline_depth\s*\(\s*0\s*\)"
            r"(?:(?!#\s*pragma\s+inline_depth\s*\(\s*\)).)*?"
            r"\bGetName\s*\(\s*\)",
            0, 0, include_directives=True),
    ),
    ("army.obj", 0x4BEEC): (
        SourceRule(
            "can_cast_spell keeps the Master Genie target test and "
            "get_valid_caliph_spells boundary in one return expression",
            r"case\s+CREATURE_MASTER_GENIE\s*:\s*return\s+target\s*&&\s*"
            r"get_valid_caliph_spells\s*\(\s*target\s*\)\s*>\s*0\s*;"),
    ),
    ("army.obj", 0x4C374): (
        SourceRule(
            "get_valid_caliph_spells keeps the 10..69 roster loop and "
            "is_valid_caliph_spell boundary",
            r"long\s+count\s*=\s*0\s*;\s*for\s*\(\s*SpellID\s+spell\s*="
            r"\s*10\s*;\s*spell\s*<\s*70\s*;\s*spell\+\+\s*\)\s*\{\s*"
            r"if\s*\(\s*is_valid_caliph_spell\s*\(\s*spell\s*,\s*target"
            r"\s*\)\s*\)\s*count\+\+\s*;\s*\}\s*return\s+count\s*;"),
    ),
    ("army.obj", 0x4C3AC): (
        SourceRule(
            "cast_caliph_spell keeps get_valid_caliph_spells before the "
            "zero guard, Random, and selection loop",
            r"SpellID\s+spell\s*;\s*long\s+count\s*=\s*"
            r"get_valid_caliph_spells\s*\(\s*target\s*\)\s*;\s*"
            r"if\s*\(\s*count\s*==\s*0\s*\)\s*return\s*;\s*"
            r"long\s+pick\s*=\s*Random\s*\(\s*1\s*,\s*count\s*\)\s*;"
            r"\s*for\s*\(\s*spell\s*=\s*10\s*;"),
    ),
    ("army.obj", 0x490F4): (
        SourceRule(
            "attacker and defender bonuses remain one total_damage expression",
            r"\btotal_damage\b\s*=\s*[^;]*"
            r"\bComputeAttackerDamageBonuses\s*\([^;]*"
            r"\bComputeDefenderDamageBonuses\s*\("),
        SourceRule(
            "get_total_hit_points stays nested in the fire-shield statement",
            r"\bcompute_fire_shield_damage\s*\([^;]*"
            r"\bget_total_hit_points\s*\("),
    ),
    ("fly.obj", 0xA1430): (
        SourceRule(
            "ValidFlight keeps the cmbtmgr.h static ValidHex boundary",
            r"\bcombatManager\s*::\s*ValidHex\s*\(\s*destIndex\s*\)"),
        SourceRule(
            "ValidFlight keeps the nested line-97/100/101 failure form",
            r"if\s*\(\s*!\s*find_flyer_attack_cell\s*\(\s*enemyHex\s*\)"
            r"\s*\)\s*\{\s*if\s*\(\s*!\s*enemy\s*->\s*Is\s*\("
            r"\s*1u\s*<<\s*0\s*\)\s*\|\|\s*!\s*"
            r"find_flyer_attack_cell\s*\(\s*"
            r"enemy\s*->\s*get_second_grid_index\s*\(\s*\)\s*\)\s*\)"
            r"\s*return\s+0\s*;"),
        SourceRule(
            "ValidFlight keeps the line-78 literal-path predicate",
            r"if\s*\(\s*bLiteralTest\s*\|\|\s*side\s*==\s*-1\s*"
            r"\|\|\s*slot\s*==\s*-1\s*\)\s*\{"),
        SourceRule(
            "ValidFlight keeps the line-95 else before the enemy path",
            r"CanFit\s*\([^;]+;\s*\}\s*else\s*\{\s*"
            r"const\s+army\s*\*\s*enemy\s*="),
    ),
    ("fly.obj", 0xA1514): (
        SourceRule(
            "FlyTo keeps the cmbtmgr.h static ValidHex boundary",
            r"\bcombatManager\s*::\s*ValidHex\s*\(\s*destIndex\s*\)"),
        SourceRule(
            "FlyTo keeps the Dreamcast-proven TestRaiseDoor boundary",
            r"\bTestRaiseDoor\s*\(\s*\)"),
    ),
    ("fly.obj", 0xA1590): (
        SourceRule(
            "Fly keeps both Dreamcast GridX statements",
            r"\bcombatManager\s*::\s*GridX\s*\(", 2),
        SourceRule(
            "Fly keeps Is(mask) and OffsetToFront in the wide-stack arm",
            r"if\s*\(\s*Is\s*\(\s*1u\s*<<\s*0\s*\)\s*&&\s*turn\s*\)"
            r"\s*destIndex\s*\+=\s*OffsetToFront\s*\(\s*-1\s*\)\s*;"),
        SourceRule("Fly keeps the iTtlLoops local identity",
                   r"\blong\s+iTtlLoops\s*="),
        SourceRule("Fly keeps the iLoop local identity and lifetime",
                   r"\blong\s+iLoop\s*;"),
        SourceRule("Fly keeps the numFlapFrames local identity",
                   r"\blong\s+numFlapFrames\s*="),
        SourceRule("Fly keeps the const int FLY_PERIOD local identity",
                   r"\bconst\s+int\s+FLY_PERIOD\s*="),
        SourceRule(
            "Fly creates TtlExtent at the top of the inner frame scope",
            r"for\s*\(\s*currFrameIndex\s*=\s*0\s*;[^)]*\)\s*\{\s*"
            r"SLimitData\s+TtlExtent\s*=\s*"
            r"gpCombatManager\s*->\s*drawbridgeBounds\s*;"),
        SourceRule(
            "Fly keeps Width and Height in the combat-area Draw statement",
            r"\bDraw\s*\([^;]*\bdrawbridgeBounds\s*\.\s*Width\s*\(\s*\)"
            r"[^;]*\bdrawbridgeBounds\s*\.\s*Height\s*\(\s*\)"),
        SourceRule(
            "Fly keeps the ScrollTo/Include/timer/UpdateCombatArea group",
            r"bool\s+scrolled\s*=\s*gpCombatManager\s*->\s*ScrollTo\s*\("
            r"[^;]*\)\s*;\s*TtlExtent\s*\.\s*Include\s*\([^;]*\)\s*;\s*"
            r"GameTime\s*::\s*DelayTil\s*\([^;]*\)\s*;\s*"
            r"glTimers\s*\[\s*0\s*\]\s*=\s*"
            r"GameTime\s*::\s*NextFrameTime\s*\([^;]*\bFLY_PERIOD\s*\)"
            r"\s*;\s*if\s*\(\s*!\s*scrolled\s*\)\s*"
            r"gpCombatManager\s*->\s*UpdateCombatArea\s*\(\s*TtlExtent\s*\)"
            r"\s*;"),
    ),
    ("fly.obj", 0xA19A0): (
        SourceRule(
            "TeleportTo keeps the cmbtmgr.h static ValidHex boundary",
            r"\bcombatManager\s*::\s*ValidHex\s*\(\s*destIndex\s*\)"),
        SourceRule(
            "TeleportTo keeps the Dreamcast-proven TestRaiseDoor boundary",
            r"\bTestRaiseDoor\s*\(\s*\)"),
    ),
    ("fly.obj", 0xA1A7C): (
        SourceRule(
            "Teleport keeps both Dreamcast GridX statements",
            r"\bcombatManager\s*::\s*GridX\s*\(", 2),
        SourceRule(
            "Teleport keeps Is(mask) and OffsetToFront in the wide-stack arm",
            r"if\s*\(\s*Is\s*\(\s*1u\s*<<\s*0\s*\)\s*&&\s*turn\s*\)"
            r"\s*destIndex\s*\+=\s*OffsetToFront\s*\(\s*-1\s*\)\s*;"),
    ),
    ("mapcell.obj", 0xF2C20): (
        SourceRule(
            "readMapObjects keeps retail's Complete-only nullary object-"
            "type-index rebuild as one explicit helper call; the Dreamcast "
            "caller omits this later-revision region",
            r"\bNewfullMapFn_005042C0\s*\(\s*\)\s*;", 1, 1),
        SourceRule(
            "readMapObjects keeps the retail-only rebuild after the "
            "readObjectType loop and before the following progress tick",
            r"\breadObjectType\s*\([^;]*\)\s*;.*?\}\s*"
            r"NewfullMapFn_005042C0\s*\(\s*\)\s*;\s*"
            r"IncProgressBar\s*\(\s*1\s*\)\s*;"),
    ),
    ("mapcell.obj", 0xF318C): (
        SourceRule(
            "loadMapObjects keeps retail's Complete-only nullary object-"
            "type-index rebuild as one explicit helper call; the Dreamcast "
            "caller omits this later-revision region",
            r"\bNewfullMapFn_005042C0\s*\(\s*\)\s*;", 1, 1),
        SourceRule(
            "loadMapObjects keeps the retail-only rebuild after the "
            "loadObjectType loop and its following progress tick",
            r"\bloadObjectType\s*\([^;]*\).*?\}\s*"
            r"IncProgressBar\s*\(\s*1\s*\)\s*;\s*"
            r"NewfullMapFn_005042C0\s*\(\s*\)\s*;"),
    ),
    ("hero.obj", 0xD4DF0): (
        SourceRule(
            "get_special_terrain keeps the get_location local",
            r"\btype_point\s+location\s*=\s*get_location\s*\(\s*\)\s*;"),
        SourceRule(
            "get_special_terrain tests its invalid sentinel through "
            "type_point::operator==",
            r"if\s*\(\s*location\s*==\s*type_point\s*\(\s*-1\s*,\s*"
            r"-1\s*,\s*-1\s*\)\s*\)"),
        SourceRule(
            "get_special_terrain keeps get_cell and the cell accessor as "
            "separate Dreamcast statements",
            r"NewmapCell\s*\*\s*cell\s*=\s*gpGame\s*->\s*get_cell\s*\("
            r"\s*location\s*\)\s*;\s*return\s+cell\s*->\s*"
            r"get_special_terrain\s*\(\s*\)\s*;"),
    ),
    ("hero.obj", 0xD4F64): (
        SourceRule(
            "GetManaCost keeps the direct GetSpellSchoolLevel source "
            "boundary after the Armageddon arm",
            r"else\s+mastery\s*=\s*GetSpellSchoolLevel\s*\("),
        SourceRule(
            "GetManaCost keeps both Dreamcast HasArmy wrappers",
            r"\bHasArmy\s*\(", 2),
    ),
    ("hero.obj", 0xD5488): (
        SourceRule(
            "Fly keeps GetManaCost nested in UseSpell after flightLevel",
            r"flightLevel\s*=\s*level\s*;\s*UseSpell\s*\(\s*"
            r"GetManaCost\s*\(\s*SPELL_FLY\s*\)\s*\)\s*;"),
    ),
    ("sacrifice_window.obj", 0x125E08): (
        SourceRule(
            "update_creature_offering keeps Dreamcast lines 955-958's "
            "two nested assignment-arm scopes",
            r"if\s*\(\s*!\s*creature\s*->\s*field_04\s*\)\s*\{\s*"
            r"result\s*=\s*convert_with_commas\s*\(\s*creature\s*->\s*"
            r"amount\s*\)\s*;\s*\}\s*else\s*\{\s*result\s*=\s*"
            r"convert_with_commas\s*\(\s*available\s*\)\s*;\s*\}"),
    ),
    ("singleselectionwindow.obj", 0x136388): (
        SourceRule(
            "SetupAdvancedOptions keeps Dreamcast's reset-loop i local "
            "inside the mapChanged scope",
            r"if\s*\(\s*mapChanged\s*\)\s*\{\s*for\s*\(\s*int\s+i\s*="
            r"\s*0\s*;\s*i\s*<\s*CNetPlayerHandler\s*::\s*MAX_PLAYERS"),
        SourceRule(
            "SetupAdvancedOptions keeps Dreamcast's function-scope "
            "nextColor and main-loop i in their recovered source order",
            r"\bint\s+nextColor\s*=\s*0\s*;.*?\bUpdateGameVars\s*\(\s*\)"
            r"\s*;\s*int\s+i\s*;\s*for\s*\(\s*i\s*=\s*0\s*;\s*i\s*<"
            r"\s*CNetPlayerHandler\s*::\s*MAX_PLAYERS"),
        SourceRule(
            "SetupAdvancedOptions keeps Dreamcast's strNbr local and "
            "compiled-out zero initializer inside the main loop",
            r"for\s*\(\s*i\s*=\s*0\s*;[^)]*\)\s*\{\s*int\s+strNbr\s*="
            r"\s*0\s*;", 1, 1),
    ),
    ("singleselectionwindow.obj", 0x13BC60): (
        SourceRule(
            "SetCurrentMap keeps Dreamcast's function-scope msg after the "
            "map-count guard",
            r"\A\s*if\s*\(\s*map\s*>=\s*static_cast\s*<\s*int\s*>\s*"
            r"\(\s*GetMapCount\s*\(\s*\)\s*\)\s*\)\s*return\s*;\s*"
            r"message\s+msg\s*;"),
        SourceRule(
            "SetCurrentMap keeps Dreamcast's bUpdate-scope widget pointer "
            "under its recovered temp name",
            r"else\s*\{\s*widget\s*\*\s*temp\s*=\s*GetWidget\s*\(\s*"
            r"101\s*\)\s*;\s*if\s*\(\s*temp\s*->\s*status\s*&\s*"
            r"widget\s*::\s*WIDGET_ACTIVE\s*\)\s*\{"),
        SourceRule(
            "SetCurrentMap keeps Dreamcast's second i, player, w and "
            "saveStatus locals in their recovered loop scopes and order",
            r"if\s*\(\s*m_flag64\s*\)\s*\{\s*for\s*\(\s*int\s+i\s*="
            r"\s*0\s*;\s*i\s*<\s*CNetPlayerHandler\s*::\s*MAX_PLAYERS"
            r"\s*;\s*\+\+\s*i\s*\)\s*\{\s*"
            r"CNetPlayerHandlerPlayer\s*\*\s*player\s*=\s*"
            r"m_players\.GetPlayerInPos\s*\(\s*i\s*\)\s*;\s*"
            r"if\s*\(\s*!\s*player\s*\)\s*player\s*=\s*"
            r"m_players\.GetCompPlayerInPos\s*\(\s*i\s*\)\s*;.*?"
            r"widget\s*\*\s*w\s*=\s*GetWidget\s*\(\s*207\s*\+\s*i"
            r"\s*\)\s*;\s*if\s*\(\s*w\s*\)\s*\{\s*"
            r"int\s+saveStatus\s*=\s*w\s*->\s*status\s*;"),
    ),
    ("singleselectionwindow.obj", 0x140D74): (
        SourceRule(
            "SendPlayerPositions keeps Dreamcast's explicit two-array "
            "CUpdatePlayerPosMsg construction before transmission",
            r"\bCUpdatePlayerPosMsg\s+msg\s*\(\s*m_players\.humanPlayers\s*,"
            r"\s*m_players\.computerPlayers\s*\)\s*;\s*"
            r"TransmitRemoteDataDPID\s*\(\s*&\s*msg\s*,\s*dpidTo\s*,"
            r"\s*true\s*,\s*true\s*\)\s*;", 1, 1),
    ),
    ("singleselectionwindow.obj", 0x143214): (
        SourceRule(
            "CanChooseTown keeps Dreamcast's mp, slotAtt, player, and "
            "isHotSeat locals in recovered order",
            r"\bCMapHeaderData\s*\*\s*mp\s*=\s*&\s*gpGame->mapHeader\s*;"
            r"\s*CMapHeaderData\s*::\s*TPlayerSlotAttributes\s*\*\s*"
            r"slotAtt\s*=.*?\bCNetPlayerHandlerPlayer\s*\*\s*player\s*="
            r".*?\bunsigned\s+char\s+isHotSeat\s*="),
        SourceRule(
            "CanChooseTown keeps both Dreamcast IsHuman calls",
            r"\bplayer\s*->\s*IsHuman\s*\(", 2, 2),
        SourceRule(
            "CanChooseTown keeps Dreamcast's self IsHost helper boundary",
            r"!\s*IsHost\s*\(\s*\)"),
    ),
    ("singleselectionwindow.obj", 0x14332C): (
        SourceRule(
            "CanChooseHero keeps Dreamcast's mp, slotAtt, player, and "
            "isHotSeat locals in recovered order",
            r"\bCMapHeaderData\s*\*\s*mp\s*=\s*&\s*gpGame->mapHeader\s*;"
            r"\s*CMapHeaderData\s*::\s*TPlayerSlotAttributes\s*\*\s*"
            r"slotAtt\s*=.*?\bCNetPlayerHandlerPlayer\s*\*\s*player\s*="
            r".*?\bunsigned\s+char\s+isHotSeat\s*="),
        SourceRule(
            "CanChooseHero keeps both Dreamcast IsHuman calls",
            r"\bplayer\s*->\s*IsHuman\s*\(", 2, 2),
        SourceRule(
            "CanChooseHero keeps Dreamcast's GetDisplayTown boundary in "
            "the rejected-town guard",
            r"if\s*\(\s*GetDisplayTown\s*\(\s*gamePos\s*\)\s*==\s*-1\s*\)"),
    ),
    ("singleselectionwindow.obj", 0x1406EC): (
        SourceRule(
            "OnGameTransmitInitMsg keeps Dreamcast lines 6807-6810's "
            "month, month-extra, week, week-extra snapshot order",
            r"\bint\s+iMonthType\s*=\s*giMonthType\s*;\s*"
            r"int\s+iMonthTypeExtra\s*=\s*giMonthTypeExtra\s*;\s*"
            r"int\s+iWeekType\s*=\s*giWeekType\s*;\s*"
            r"int\s+iWeekTypeExtra\s*=\s*giWeekTypeExtra\s*;"),
        SourceRule(
            "OnGameTransmitInitMsg keeps Dreamcast lines 6818-6828's "
            "calendar restoration, watch-player, visibility-bit, "
            "player-turn, duration-update order in Complete's retail-"
            "proved globals",
            r"\bgiMonthType\s*=\s*iMonthType\s*;\s*"
            r"giMonthTypeExtra\s*=\s*iMonthTypeExtra\s*;\s*"
            r"giWeekType\s*=\s*iWeekType\s*;\s*"
            r"giWeekTypeExtra\s*=\s*iWeekTypeExtra\s*;\s*"
            r"gUnnamed69778c\s*=\s*gLocalGamePos\s*;\s*"
            r"gMapVisibilityBit\s*=\s*1\s*<<\s*gLocalGamePos\s*;\s*"
            r"gUnnamed69d810\s*=\s*gNetLocalGamePos\s*;\s*"
            r"UpdateTurnDuration\s*\(\s*\)\s*;"),
    ),
}


# One continuous out-of-class header-inline roster in Dreamcast Army.h.  The
# class LF_FIELDLIST (type 0x205b) contains their declarations interspersed
# among the other members, while CodeView lines 718..881 identify this later
# definition run.  Conflating those two facts was the local-minimum defect this
# gate is specifically meant to prevent.
ARMY_HEADER_ROSTER: tuple[tuple[int, str, str], ...] = (
    (718, "can_cast_resurrect",
     r"bool\s+can_cast_resurrect\s*\(\s*\)\s*const"),
    (724, "GetMorale",
     r"int\s+GetMorale\s*\(\s*unsigned\s+char\s+\w+\s*\)\s*const"),
    (730, "GetLuck",
     r"int\s+GetLuck\s*\(\s*unsigned\s+char\s+\w+\s*\)\s*const"),
    (736, "OffsetToFront",
     r"int\s+OffsetToFront\s*\(\s*int\s+\w+\s*\)\s*const"),
    (752, "clear_AI_values", r"void\s+clear_AI_values\s*\(\s*\)"),
    (760, "NeedToTurn",
     r"bool\s+NeedToTurn\s*\(\s*int\s+\w+\s*\)\s*const"),
    (765, "Is",
     r"bool\s+Is\s*\(\s*unsigned\s+\w+\s*\)\s*const"),
    (770, "get_AI_expected_damage",
     r"long\s+get_AI_expected_damage\s*\(\s*\)\s*const"),
    (775, "get_AI_target",
     r"const\s+army\s*\*\s*get_AI_target\s*\(\s*\)\s*const"),
    (780, "get_AI_target_value",
     r"long\s+get_AI_target_value\s*\(\s*\)\s*const"),
    (785, "get_AI_target_time",
     r"long\s+get_AI_target_time\s*\(\s*\)\s*const"),
    (790, "get_AI_possible_targets",
     r"long\s+get_AI_possible_targets\s*\(\s*\)\s*const"),
    (795, "get_owning_side", r"int\s+get_owning_side\s*\(\s*\)\s*const"),
    (800, "get_controlling_side",
     r"int\s+get_controlling_side\s*\(\s*\)\s*const"),
    (810, "GetName", r"const\s+char\s*\*\s*GetName\s*\(\s*\)\s*const"),
    (815, "GetName(count)",
     r"const\s+char\s*\*\s*GetName\s*\(\s*int\s+\w+\s*\)\s*const"),
    (820, "get_spell_time",
     r"long\s+get_spell_time\s*\(\s*int\s+\w+\s*\)\s*const"),
    (825, "get_spell_level",
     r"int\s+get_spell_level\s*\(\s*int\s+\w+\s*\)\s*const"),
    (830, "IsActive", r"bool\s+IsActive\s*\(\s*\)\s*const"),
    (835, "is_in_aura",
     r"bool\s+is_in_aura\s*\(\s*\)\s*const"),
    (840, "IsIncapacitated",
     r"bool\s+IsIncapacitated\s*\(\s*\)\s*const"),
    (847, "can_retaliate",
     r"bool\s+can_retaliate\s*\(\s*const\s+army\s*&\s*\w+\s*\)\s*const"),
    (855, "cannot_attack",
     r"bool\s+cannot_attack\s*\(\s*\)\s*const"),
    (864, "get_adjacent_hex(direction)",
     r"long\s+get_adjacent_hex\s*\(\s*long\s+\w+\s*\)\s*const"),
    (869, "get_attack_direction(enemy)",
     r"long\s+get_attack_direction\s*\(\s*const\s+army\s*\*\s*\w+\s*\)\s*const"),
    (875, "LeavesNoBody",
     r"bool\s+LeavesNoBody\s*\(\s*\)\s*const"),
    (881, "is_in_area_highlight",
     r"bool\s+is_in_area_highlight\s*\(\s*\)\s*const"),
)

ARMY_HEADER_BODY_RULES: dict[int, tuple[SourceRule, ...]] = {
    718: (SourceRule("resurrect capability keeps both creature tests",
                     r"creatureType\s*==\s*ARMY_CREATURE_ARCHANGEL[^;]*creatureType\s*==\s*ARMY_CREATURE_PIT_LORD", 2),
          SourceRule("resurrect capability tests numSpellCasts",
                     r"numSpellCasts\s*>\s*0", 2)),
    724: (SourceRule("GetMorale conditionally calls limit(-3,morale,3)",
                     r"apply_limits\s*\?\s*limit\s*\(\s*-3\s*,\s*morale\s*,\s*3\s*\)\s*:\s*morale"),),
    730: (SourceRule("GetLuck conditionally calls limit(-3,luck,3)",
                     r"apply_limits\s*\?\s*limit\s*\(\s*-3\s*,\s*luck\s*,\s*3\s*\)\s*:\s*luck"),),
    752: (SourceRule("clear_AI_values preserves its four-store order",
                     r"AI_expected_damage\s*=\s*0\s*;\s*AI_target\s*=\s*0\s*;\s*AI_target_time\s*=\s*0\s*;\s*AI_target_value\s*=\s*0\s*;"),),
    760: (SourceRule("NeedToTurn retains the combined predicate",
                     r"direction\s*<\s*6\s*&&\s*\(\s*facing\s*==\s*0\s*\)\s*!=\s*\(\s*direction\s*>=\s*3\s*\)"),),
    770: (SourceRule("get_AI_expected_damage returns its member",
                     r"return\s+AI_expected_damage\s*;"),),
    775: (SourceRule("get_AI_target returns its member",
                     r"return\s+AI_target\s*;"),),
    780: (SourceRule("get_AI_target_value returns its member",
                     r"return\s+AI_target_value\s*;"),),
    785: (SourceRule("nullary AI time calls GetSpeed then its overload",
                     r"return\s+get_AI_target_time\s*\(\s*GetSpeed\s*\(\s*\)\s*\)\s*;"),),
    790: (SourceRule("get_AI_possible_targets returns its member",
                     r"return\s+AI_possible_targets\s*;"),),
    795: (SourceRule("get_owning_side returns combatSide",
                     r"return\s+combatSide\s*;"),),
    800: (SourceRule("get_controlling_side tests Hypnotize's row",
                     r"spellInfluence\s*\[\s*60\s*\]"),
          SourceRule("get_controlling_side keeps both owning-side calls",
                     r"get_owning_side\s*\(", 2)),
    810: (SourceRule("GetName() calls GetArmyName with numTroops",
                     r"GetArmyName\s*\(\s*creatureType\s*,\s*numTroops\s*\)"),),
    815: (SourceRule("GetName(count) forwards count",
                     r"GetArmyName\s*\(\s*creatureType\s*,\s*count\s*\)"),),
    820: (SourceRule("get_spell_time indexes spellInfluence",
                     r"return\s+spellInfluence\s*\[\s*spell\s*\]\s*;"),),
    825: (SourceRule("get_spell_level indexes spell_level",
                     r"return\s+spell_level\s*\[\s*spell\s*\]\s*;"),),
    830: (SourceRule("IsActive retains type/count predicate",
                     r"creatureType\s*>=\s*0\s*&&\s*numTroops\s*>\s*0"),),
    835: (SourceRule("is_in_aura calls aura_sources.size",
                     r"aura_sources\s*\.\s*size\s*\(\s*\)"),),
    840: (SourceRule("IsIncapacitated keeps all three spell rows",
                     r"spellInfluence\s*\[\s*62\s*\][^;]*spellInfluence\s*\[\s*70\s*\][^;]*spellInfluence\s*\[\s*74\s*\]"),),
    847: (SourceRule("can_retaliate keeps attacker's Is(mask) call",
                     r"attacker\s*\.\s*Is\s*\(\s*1u\s*<<\s*16\s*\)"),
          SourceRule("can_retaliate keeps Stone and retaliation tests",
                     r"spellInfluence\s*\[\s*70\s*\][^;]*retaliationCount\s*>\s*0")),
    855: (SourceRule("cannot_attack keeps incapacity, mask, and both ids",
                     r"IsIncapacitated\s*\(\s*\)[^;]*Is\s*\(\s*1u\s*<<\s*21\s*\)[^;]*creatureType\s*==\s*ARMY_CREATURE_PSYCHIC_ELEMENTAL[^;]*creatureType\s*==\s*ARMY_CREATURE_MAGIC_ELEMENTAL"),),
    864: (SourceRule("one-argument get_adjacent_hex forwards gridIndex",
                     r"get_adjacent_hex\s*\(\s*gridIndex\s*,\s*direction\s*\)"),),
    869: (SourceRule("one-argument attack direction forwards gridIndex",
                     r"get_attack_direction\s*\(\s*gridIndex\s*,\s*enemy\s*\)"),),
    875: (SourceRule("LeavesNoBody retains its combined Is mask",
                     r"Is\s*\(\s*\(\s*1u\s*<<\s*22\s*\)\s*\|\s*\(\s*1u\s*<<\s*28\s*\)\s*\)"),),
    881: (SourceRule("is_in_area_highlight returns its byte member",
                     r"return\s+is_area_effect_target\s*;"),),
}


# Dreamcast type 0x1a95 / LF_FIELDLIST 0x205b entries 97..236.  Overload
# groups occupy one field-list entry and therefore appear once.  Complete may
# add members, but every shared declaration must retain this relative order;
# a current retail percentage is never a waiver for changing it.
ARMY_PUBLIC_METHOD_ORDER: tuple[str, ...] = (
    "army", "Init", "initialize", "InitClean", "LoadResources",
    "FreeResources", "ResetRound", "EndWalk", "Walk", "WalkTo", "Fly",
    "FlyTo", "Teleport", "TeleportTo", "adjust_damage",
    "adjust_hitpoints", "attack_hex", "do_attack", "do_multi_head_attack",
    "range_attack", "AttackWall", "Turn", "NeedToTurn",
    "can_cast_resurrect", "can_cast_spell", "can_retaliate", "can_shoot",
    "cast_caliph_spell", "cast_resurrect", "cast_demonic_resurrect",
    "check_special_attack", "cast_spell", "check_obstacle_attacks",
    "clear_AI_values", "consider_attack", "enemy_is_adjacent",
    "GetAttackMask", "get_adjusted_attack", "get_adjusted_defense",
    "get_AI_expected_damage", "get_AI_target", "get_AI_target_value",
    "get_AI_target_time", "get_AI_possible_targets", "get_attack_modifier",
    "get_average_damage", "get_berserk_targets", "get_owning_side",
    "get_controlling_side", "get_owner", "get_controller",
    "get_defense_damage_modifier", "get_defense_modifier", "get_clockwise",
    "get_counter_clockwise", "get_fire_shield_strength",
    "get_loss_combat_value", "get_resurrection_size",
    "get_second_grid_index", "get_total_combat_value",
    "get_total_hit_points", "get_unit_combat_value",
    "get_valid_caliph_spells", "GetBestDirection", "is_adjacent", "is_enemy",
    "is_in_aura", "move_to", "new_turn", "set_AI_expected_damage",
    "FindPath", "set_retaliation_count", "ValidAttack", "ResetPath",
    "ValidPath", "ValidFlight", "ValidRange", "DamageEnemy", "Damage",
    "ComputeBaseDamage", "ComputeAttackerDamageBonuses",
    "ComputeDefenderDamageBonuses", "ComputeAttackerDamageReduction",
    "ComputeDefenderDamageReduction", "CancelSpellType",
    "DecrementSpellRounds", "GoBerserk", "Cure", "CanFit", "DrawToBuffer",
    "PlayAnimation", "SetupAnimation", "Strength", "IsActive", "CheckLuck",
    "SetSpellInfluence", "CancelIndividualSpell", "CancelAllSpells", "BottomY",
    "MidY", "TopY", "LeftX", "RightX", "FrontX", "MidX", "Is",
    "is_in_area_highlight", "OffsetToFront", "ProcessDeath",
    "OtherArmyAdjacent", "GetAdjacentCellIndex", "get_adjacent_hex",
    "get_attack_direction", "get_multi_head_directions", "get_spell_time",
    "get_spell_level", "set_inside_area_effect", "play_sample", "stop_sample",
    "WaitSample", "add_aura", "remove_aura", "remove_binding", "cannot_attack",
    "GetName", "IsIncapacitated", "SetLuck", "GetLuck", "SetMorale",
    "GetMorale", "GetSpeed",
)
ARMY_PRIVATE_METHOD_ORDER: tuple[str, ...] = (
    "animate_missile", "attack_wall", "do_fire_shield", "do_post_attack",
    "DoHydraAttack", "find_flyer_attack_cell", "LeavesNoBody", "simple_move",
    "ComputeKarma",
)
ARMY_PRIVATE_TAIL: tuple[tuple[str, str], ...] = (
    ("iMorale", "morale"),
    ("iLuck", "luck"),
    ("reset_this_round", "field_4f0"),
    ("is_area_effect_target", "is_area_effect_target"),
    ("bound_armies", "bound_armies"),
    ("binders", "binders"),
    ("aura_clients", "aura_clients"),
    ("aura_sources", "aura_sources"),
    ("AI_expected_damage", "AI_expected_damage"),
    ("AI_target", "AI_target"),
    ("AI_target_value", "AI_target_value"),
    ("AI_target_distance", "AI_target_time"),
    ("AI_possible_targets", "AI_possible_targets"),
)

# S_PUB32 decorated identities use `_N` for these predicates.  Checking the
# class declarations as well as the later inline bodies prevents a stale
# unsigned-char declaration from silently re-mangling every surviving COMDAT.
ARMY_BOOL_DECLARATIONS: tuple[SourceRule, ...] = (
    SourceRule("both can_cast_resurrect overloads return bool",
               r"\bbool\s+can_cast_resurrect\s*\(", 2),
    SourceRule("NeedToTurn returns bool", r"\bbool\s+NeedToTurn\s*\("),
    SourceRule("Is returns bool", r"\bbool\s+Is\s*\("),
    SourceRule("can_retaliate returns bool", r"\bbool\s+can_retaliate\s*\("),
    SourceRule("IsActive returns bool", r"\bbool\s+IsActive\s*\("),
    SourceRule("is_in_aura returns bool", r"\bbool\s+is_in_aura\s*\("),
    SourceRule("cannot_attack returns bool", r"\bbool\s+cannot_attack\s*\("),
    SourceRule("IsIncapacitated returns bool",
               r"\bbool\s+IsIncapacitated\s*\("),
    SourceRule("is_in_area_highlight returns bool",
               r"\bbool\s+is_in_area_highlight\s*\("),
    SourceRule("both find_flyer_attack_cell overloads return bool",
               r"\bbool\s+find_flyer_attack_cell\s*\(", 2),
    SourceRule("LeavesNoBody returns bool", r"\bbool\s+LeavesNoBody\s*\("),
)


def combat_manager_header_violations(text: str) -> list[tuple[int, str]]:
    """Audit the attested cmbtmgr.h inline signature/body/order band."""
    masked = _source.mask(text)
    rules: tuple[tuple[str, str, str], ...] = (
        ("ValidHex", "cmbtmgr.h:1460 ValidHex must remain static bool with "
         "the attested two-bound predicate",
         r"\bstatic\s+bool\s+ValidHex\s*\(\s*int\s+(\w+)\s*\)\s*\{"
         r"\s*return\s+\1\s*>=\s*0\s*&&\s*\1\s*<\s*"
         r"COMBAT_GRID_CELLS\s*;\s*\}"),
        ("get_current_army", "cmbtmgr.h:1478 get_current_army must return "
         "the acting-side/slot army",
         r"\barmy\s*\*\s*get_current_army\s*\(\s*\)\s*\{\s*"
         r"return\s*&\s*armies\s*\[\s*actingSide\s*\]\s*"
         r"\[\s*actingSlot\s*\]\s*;\s*\}"),
        ("GetHexIndex", "cmbtmgr.h:1500 GetHexIndex must remain static int "
         "with the row-stride expression",
         r"\bstatic\s+int\s+GetHexIndex\s*\(\s*int\s+(\w+)\s*,\s*"
         r"int\s+(\w+)\s*\)\s*\{\s*return\s+\2\s*\*\s*"
         r"COMBAT_GRID_ROW_STRIDE\s*\+\s*\1\s*;\s*\}"),
        ("RowIsOdd", "cmbtmgr.h:1506 RowIsOdd must remain a const bool "
         "member with its low-bit test",
         r"\bbool\s+RowIsOdd\s*\(\s*int\s+(\w+)\s*\)\s*const\s*\{"
         r"\s*return\s*\(\s*\1\s*&\s*1\s*\)\s*!=\s*0\s*;\s*\}"),
        ("GridX", "cmbtmgr.h:1519 GridX must remain static int with its "
         "row-stride remainder",
         r"\bstatic\s+int\s+GridX\s*\(\s*int\s+(\w+)\s*\)\s*\{"
         r"\s*return\s+\1\s*%\s*COMBAT_GRID_ROW_STRIDE\s*;\s*\}"),
        ("InInvisibleColumn", "cmbtmgr.h:1525 InInvisibleColumn must remain "
         "static bool and call ValidHex then GridX",
         r"\bstatic\s+bool\s+InInvisibleColumn\s*\(\s*int\s+(\w+)\s*\)"
         r"\s*\{\s*if\s*\(\s*!\s*ValidHex\s*\(\s*\1\s*\)\s*\)"
         r"\s*return\s+false\s*;\s*int\s+(\w+)\s*=\s*GridX\s*\(\s*"
         r"\1\s*\)\s*;\s*return\s+\2\s*==\s*0\s*\|\|\s*\2\s*==\s*"
         r"COMBAT_GRID_LAST_COLUMN\s*;\s*\}"),
        ("GetCell", "cmbtmgr.h:1537 GetCell must return hexcell& through "
         "GetHexIndex",
         r"\bhexcell\s*&\s*GetCell\s*\(\s*int\s+(\w+)\s*,\s*int\s+"
         r"(\w+)\s*\)\s*\{\s*return\s+cells\s*\[\s*GetHexIndex\s*\("
         r"\s*\1\s*,\s*\2\s*\)\s*\]\s*;\s*\}"),
        ("GetObstacle", "cmbtmgr.h:1542 GetObstacle must return TObstacle& "
         "from obstacles.begin",
         r"\bTObstacle\s*&\s*GetObstacle\s*\(\s*int\s+(\w+)\s*\)\s*"
         r"\{\s*return\s+obstacles\.begin\s*\[\s*\1\s*\]\s*;\s*\}"),
        ("TestRaiseDoor", "Complete's inline TestRaiseDoor boundary must "
         "remain a wrapper around RaiseDoor",
         r"\bvoid\s+TestRaiseDoor\s*\(\s*\)\s*\{\s*RaiseDoor\s*\(\s*\)"
         r"\s*;\s*\}"),
    )
    defects: list[tuple[int, str]] = []
    positions: list[tuple[int, str]] = []
    for name, description, pattern in rules:
        match = re.search(pattern, masked, re.DOTALL)
        if match is None:
            token = re.search(r"\b" + re.escape(name) + r"\s*\(", masked)
            line = text.count("\n", 0, token.start()) + 1 if token else 1
            defects.append((line, description))
        elif name != "TestRaiseDoor":
            positions.append((match.start(), name))
    expected = [name for name, _description, _pattern in rules
                if name != "TestRaiseDoor"]
    actual = [name for _position, name in sorted(positions)]
    if len(actual) == len(expected) and actual != expected:
        first = next(index for index, names in enumerate(zip(actual, expected))
                     if names[0] != names[1])
        position = sorted(positions)[first][0]
        defects.append((text.count("\n", 0, position) + 1,
                        "cmbtmgr.h inline order must remain "
                        + " -> ".join(expected)))
    return defects


def type_point_header_violations(text: str) -> list[tuple[int, str]]:
    """Audit struct.h's Dreamcast-proven equality operator source shape."""
    masked = _source.mask(text)
    pattern = (
        r"\bbool\s+operator\s*==\s*\(\s*const\s+type_point\s*&\s*"
        r"(\w+)\s*\)\s*const\s*\{\s*return\s+x\s*==\s*\1\.x\s*&&\s*"
        r"y\s*==\s*\1\.y\s*&&\s*z\s*==\s*\1\.z\s*;\s*\}")
    match = re.search(pattern, masked, re.DOTALL)
    if match is not None:
        return []
    token = re.search(r"\boperator\s*==\s*\(", masked)
    line = text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "struct.h type_point::operator== must remain a bool const "
             "member taking const type_point& and compare x, y, z in order")]


def hero_get_location_header_violations(text: str) -> list[tuple[int, str]]:
    """Audit Hero.h:157's ordinary in-class packed-point accessor."""
    masked = _source.mask(text)
    pattern = (
        r"(?m)^[ \t]{4}type_point\s+get_location\s*\(\s*\)\s*const"
        r"\s*\{\s*return\s+type_point\s*\(\s*x\s*,\s*y\s*,\s*z\s*"
        r"\)\s*;\s*\}")
    if re.search(pattern, masked, re.DOTALL) is not None:
        return []
    token = re.search(r"\bget_location\s*\(\s*\)\s*const", masked)
    line = text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "Hero.h:157 type_obscuring_object::get_location must remain "
             "an ordinary in-class const type_point construction from "
             "x, y, z; __forceinline changes the attested callers' VC6 "
             "inliner state")]


def hero_get_target_header_violations(text: str) -> list[tuple[int, str]]:
    """Audit Hero.h:986's packed-point helper boundary and field order."""
    masked = _source.mask(text)
    pattern = (
        r"\b__forceinline\s+type_point\s+get_target\s*\(\s*\)\s*const"
        r"\s*\{\s*return\s+type_point\s*\(\s*pathTargetX\s*,\s*"
        r"pathTargetY\s*,\s*pathTargetZ\s*\)\s*;\s*\}")
    if re.search(pattern, masked, re.DOTALL) is not None:
        return []
    token = re.search(r"\bget_target\s*\(", masked)
    line = text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "Hero.h:986 hero::get_target must retain Dreamcast's const "
             "inline type_point construction from pathTargetX, Y, Z")]


def cmc_hide_hero_header_violations(text: str) -> list[tuple[int, str]]:
    """Audit the Dreamcast-proven CNetMsg -> CMapChange -> hide chain."""
    masked = _source.mask(text)
    specs = (
        (
            "netmsg.h eRS_Messages must retain the scoped Dreamcast lobby "
            "roster and Complete's three trailing transfer rungs",
            r"RS_GAME_HEADER_INFO\s*=\s*1023\s*,\s*"
            r"RS_GAME_HEADER_INFO_INIT\s*=\s*1024\s*,\s*"
            r"RS_GAME_HEADER_INFO_END\s*=\s*1025\s*,\s*"
            r"RS_NEW_SETUP_INFO\s*=\s*1026\s*,\s*RS_SCROLL\s*=\s*1027"
            r"\s*,\s*RS_NEW_MAP_HEADER_INFO\s*=\s*1028\s*,\s*"
            r"RS_MAP_HEADER_REQUEST\s*=\s*1029\s*,\s*RS_MAP_FILE_NAME"
            r"\s*=\s*1030\s*,\s*RS_SORT_MAPS\s*=\s*1031\s*,\s*"
            r"RS_SET_FILTER\s*=\s*1032\s*,\s*RS_REQUEST_HERO_FACE\s*="
            r"\s*1035\s*,\s*RS_REQUEST_HERO_FACE_REPLY\s*=\s*1036\s*,"
            r"\s*RS_SETAGR\s*=\s*1037\s*,\s*RS_NEW_HOST\s*=\s*1038"
            r"\s*,\s*RS_UPDATE_PLAYER_POS\s*=\s*1039\s*,\s*"
            r"RS_NEW_PLAYER\s*=\s*1040\s*,\s*RS_REQ_HEADER_CONFIRM\s*="
            r"\s*1041\s*,\s*RS_HEADER_CONFIRM\s*=\s*1042\s*,\s*"
            r"RS_CLICK\s*=\s*1043\s*,\s*RS_TOWN_UPDATE\s*=\s*1044\s*,"
            r"\s*RS_LAUNCHING_GAME\s*=\s*1045\s*,\s*RS_BAD_VERSION\s*="
            r"\s*1046\s*,\s*RS_GAME_TRANSMIT_PENDING\s*=\s*1082\s*,"
            r"\s*RS_GAME_HEADER_INFO_INIT_EX\s*=\s*1083\s*,\s*"
            r"RS_HEADERS_REQUEST\s*=\s*1084\s*,",
            r"\bRS_GAME_HEADER_INFO\b"),
        (
            "netmsg.h:167 CNetMsg must retain Dreamcast's eRS_Messages "
            "subType and unsigned-long size parameters plus its five "
            "ordered body assignments",
            r"\bCNetMsg\s*\(\s*eRS_Messages\s+subType\s*,\s*"
            r"unsigned\s+long\s+size\s*\)\s*\{\s*this\s*->\s*subType"
            r"\s*=\s*subType\s*;\s*field_00\s*=\s*-\s*1\s*;\s*"
            r"this\s*->\s*size\s*=\s*size\s*;\s*field_04\s*=\s*0"
            r"\s*;\s*field_10\s*=\s*0\s*;\s*\}",
            r"\bCNetMsg\s*\("),
        (
            "netmsg.h:532 CMapChange must retain Dreamcast's id and size "
            "parameters and distinct CNetMsg(id, size) construction "
            "boundary",
            r"\bCMapChange\s*\(\s*eRS_Messages\s+id\s*,\s*unsigned\s+"
            r"long\s+size\s*\)\s*:\s*CNetMsg\s*\(\s*id\s*,\s*size"
            r"\s*\)\s*\{\s*\}",
            r"\bCMapChange\s*\("),
        (
            "netmsg.h:717 CMCHideHero must retain Dreamcast's int heroId "
            "parameter and CMapChange base-constructor boundary before the "
            "separate shadowed heroId body assignment",
            r"\bclass\s+CMCHideHero\s*:\s*public\s+CMapChange\s*\{.*?"
            r"\bint\s+heroId\s*;.*?\bCMCHideHero\s*\(\s*int\s+heroId"
            r"\s*\)\s*:\s*CMapChange\s*\(\s*RS_HIDE_HERO\s*,\s*"
            r"sizeof\s*\(\s*CMCHideHero\s*\)\s*\)\s*\{\s*this\s*"
            r"->\s*heroId\s*=\s*heroId\s*;\s*\}",
            r"\bCMCHideHero\s*\("),
    )
    defects: list[tuple[int, str]] = []
    for description, pattern, token_pattern in specs:
        if re.search(pattern, masked, re.DOTALL) is not None:
            continue
        token = re.search(token_pattern, masked)
        line = text.count("\n", 0, token.start()) + 1 if token else 1
        defects.append((line, description))
    return defects


def cmc_claim_header_violations(text: str) -> list[tuple[int, str]]:
    """Audit the two NB11 base-then-member claim message constructors."""
    masked = _source.mask(text)
    specs = (
        ("CMCClaimMine", "mineId", "RS_CLAIM_MINE"),
        ("CMCClaimTown", "townId", "RS_CLAIM_TOWN"),
    )
    defects: list[tuple[int, str]] = []
    for class_name, id_member, subtype in specs:
        pattern = (
            r"\bclass\s+" + class_name +
            r"\s*:\s*public\s+CMapChange\s*\{.*?\bsigned\s+char\s+" +
            id_member + r"\s*;\s*int\s+playerPos\s*;.*?\b" + class_name +
            r"\s*\(\s*signed\s+char\s+id\s*,\s*int\s+player\s*\)\s*"
            r":\s*CMapChange\s*\(\s*" + subtype +
            r"\s*,\s*sizeof\s*\(\s*" + class_name +
            r"\s*\)\s*\)\s*\{\s*" + id_member +
            r"\s*=\s*id\s*;\s*playerPos\s*=\s*player\s*;\s*\}")
        if re.search(pattern, masked, re.DOTALL) is not None:
            continue
        token = re.search(r"\b" + class_name + r"\s*\(", masked)
        line = text.count("\n", 0, token.start()) + 1 if token else 1
        defects.append((
            line,
            "netmsg.h " + class_name + " must retain Dreamcast's "
            "CMapChange base-constructor boundary followed by separate " +
            id_member + " and playerPos body assignments"))
    return defects


def event_record_constructor_violations(text: str) -> list[tuple[int, str]]:
    """Audit recovered event_record constructor definition sites."""
    marks = list(PROVENANCE_RE.finditer(text))
    specs = (
        (96,
         "event_record.cpp:96 move-record constructor must remain an "
         "ordinary inline definition at its recovered source position",
         r"\A\s*inline\s+type_record_move_hero\s*::\s*"
         r"type_record_move_hero\s*\(\s*hero\s*\*\s*_hero\s*,\s*"
         r"char\s+_direction\s*,\s*type_point\s+_destination\s*\)\s*"
         r"\{"),
        (204,
         "event_record.cpp:204 teleport constructor must remain an ordinary "
         "inline definition delegating to type_record_move_hero with the "
         "hero's facing byte",
         r"\A\s*inline\s+type_record_teleport\s*::\s*"
         r"type_record_teleport\s*\(\s*hero\s*\*\s*_hero\s*,\s*"
         r"type_point\s+_destination\s*\)\s*:\s*"
         r"type_record_move_hero\s*\(\s*_hero\s*,\s*"
          r"_hero\s*->\s*facing\s*,\s*_destination\s*\)\s*\{\s*\}"),
        (237,
         "event_record.cpp:237 claim-mine constructor must remain an "
         "ordinary inline definition at its recovered source position",
         r"\A\s*inline\s+type_record_claim_mine\s*::\s*"
         r"type_record_claim_mine\s*\(\s*long\s+_id\s*,\s*char\s+"
         r"_new_owner\s*\)\s*\{"),
        (321,
         "event_record.cpp:321 claim-town constructor must remain an "
         "ordinary inline definition at its recovered source position and "
         "use Dreamcast's default claim-mine base boundary",
         r"\A\s*inline\s+type_record_claim_town\s*::\s*"
         r"type_record_claim_town\s*\(\s*long\s+_id\s*,\s*char\s+"
         r"_new_owner\s*\)\s*:\s*type_record_claim_mine\s*\(\s*\)"
         r"\s*\{"),
    )
    defects: list[tuple[int, str]] = []
    for original_line, description, pattern in specs:
        selected = None
        selected_index = -1
        for index, mark in enumerate(marks):
            source = mark.group(1).replace("/", "\\").lower()
            if source.endswith("\\event_record.cpp") \
                    and int(mark.group(2)) == original_line:
                selected = mark
                selected_index = index
                break
        if selected is None:
            defects.append((1, description))
            continue
        end = marks[selected_index + 1].start() \
            if selected_index + 1 < len(marks) else len(text)
        segment = _source.mask(text[selected.end():end])
        if re.search(pattern, segment, re.DOTALL) is None:
            defects.append((text.count("\n", 0, selected.start()) + 1,
                            description))
    return defects


def recruit_inline_contract_violations(
        source_text: str, recruit_header_text: str,
        message_header_text: str) -> list[tuple[int, str]]:
    """Audit recruit.obj's two Dreamcast-proven inline source boundaries.

    Raw NB11 records ``message::message`` as Update's first body statement.
    The retail prologue independently contains its eight ordered zero stores,
    and both other recruit.obj message users remain exact under the same
    compiland view.  UpdateCost is separately recorded in recruit.cpp with a
    sole ``resCost`` local and a GetMonsterCost call; retail expands that
    nested helper chain at every call site and emits no standalone body.
    """
    source = _source.mask(source_text)
    recruit_header = _source.mask(recruit_header_text)
    message_header = _source.mask(message_header_text)
    defects: list[tuple[int, str]] = []

    constructor_view = re.search(
        r"#\s*define\s+HOMM3_RECRUIT_MESSAGE_CTOR_VIEW\s*\r?\n"
        r"\s*#\s*include\s+\"message\.h\"", source_text) is not None
    constructor_guard = re.search(
        r"defined\s*\(\s*HOMM3_RECRUIT_MESSAGE_CTOR_VIEW\s*\)",
        message_header_text) is not None
    constructor_body = re.search(
        r"\bmessage\s*\(\s*\)\s*\{\s*id\s*=\s*0\s*;\s*"
        r"codeX\s*=\s*0\s*;\s*codeY\s*=\s*0\s*;\s*"
        r"qualifier\s*=\s*0\s*;\s*mouseX\s*=\s*0\s*;\s*"
        r"mouseY\s*=\s*0\s*;\s*extra\s*=\s*0\s*;\s*"
        r"window\s*=\s*0\s*;\s*\}", message_header,
        re.DOTALL) is not None
    if not (constructor_view and constructor_guard and constructor_body):
        token = re.search(r"\bmessage\s+msg\s*;", source)
        line = source_text.count("\n", 0, token.start()) + 1 if token else 1
        defects.append((line,
            "recruit.cpp must retain its TU-scoped message constructor view "
            "and message.h's ordered eight-field constructor body"))

    inline_declaration = re.search(
        r"\binline\s+void\s+UpdateCost\s*\(\s*\)\s*;",
        recruit_header) is not None
    inline_definition = re.search(
        r"\binline\s+void\s+recruitUnit\s*::\s*UpdateCost\s*"
        r"\(\s*\)\s*\{\s*int\s+resCost\s*\[\s*7\s*\]\s*;\s*"
        r"GetMonsterCost\s*\(\s*monsterType\s*,\s*resCost\s*\)\s*;",
        source, re.DOTALL) is not None
    if not (inline_declaration and inline_definition):
        token = re.search(r"\bUpdateCost\s*\(", source)
        line = source_text.count("\n", 0, token.start()) + 1 if token else 1
        defects.append((line,
            "recruitUnit::UpdateCost must retain its inline recruit.cpp "
            "definition after GetMonsterCost, with the resCost/helper head"))
    return defects


def game_get_hero_header_violations(text: str) -> list[tuple[int, str]]:
    """Audit Game.h:972's no-call GetHero inline source shape.

    CodeView supplies the ``which`` parameter and the three source rows: the
    -1 branch, its null return, then the hero-array pointer return.  The SH4
    lowering forms the member base before adding the scaled index, but cannot
    distinguish ``heroes + which`` from ``&heroes[which]``; both spellings are
    therefore accepted by the proof gate.
    """
    masked = _source.mask(text)
    pattern = (
        r"\bhero\s*\*\s*GetHero\s*\(\s*int\s+which\s*\)\s*\{\s*"
        r"if\s*\(\s*which\s*==\s*-\s*1\s*\)\s*return\s+0\s*;\s*"
        r"return\s+(?:heroes\s*\+\s*which|&\s*heroes\s*\[\s*which\s*"
        r"\])\s*;\s*\}")
    if re.search(pattern, masked, re.DOTALL) is not None:
        return []
    token = re.search(r"\bGetHero\s*\(", masked)
    line = text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "Game.h:972 game::GetHero must retain Dreamcast's int which "
             "parameter, -1 null arm, and direct heroes-index return")]


def game_is_computer_team_header_violations(
        text: str) -> list[tuple[int, str]]:
    """Audit the DC boundary and Complete-proven IsComputerTeam lowering."""
    masked = _source.mask(text)
    pattern = (
        r"\binline\s+unsigned\s+char\s+IsComputerTeam\s*\(\s*"
        r"int\s+teamNum\s*\)\s*const\s*\{\s*"
        r"if\s*\(\s*teamNum\s*<\s*0\s*\)\s*return\s+0\s*;\s*"
        r"return\s+!\s*is_human_ally\s*\(\s*teamNum\s*\)\s*;\s*\}")
    if re.search(pattern, masked, re.DOTALL) is not None:
        return []
    token = re.search(r"\bIsComputerTeam\s*\(", masked)
    line = text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "Game.h:856 game::IsComputerTeam must retain Dreamcast's "
             "unsigned-char const inline boundary and negative-team arm, "
             "with Complete's retail-proven is_human_ally negation")]


def game_randomize_header_violations(text: str) -> list[tuple[int, str]]:
    """Audit the compatible tail order from game LF_FIELDLIST 0x3edc.

    Dreamcast marks both members private, but retail's public decorations
    directly reject that access flag.  The relative declaration order and
    the randomize_university parameter survive both revisions; Complete-only
    declarations are explicitly allowed between the two shared members.
    """
    masked = _source.mask(text)
    pattern = (
        r"\bvoid\s+match_underground_gates\s*\(\s*\)\s*;.*?"
        r"\bvoid\s+randomize_university\s*\(\s*"
        r"NewmapCell\s*\*\s*cell\s*\)\s*;")
    if re.search(pattern, masked, re.DOTALL) is not None:
        return []
    token = re.search(r"\brandomize_university\s*\(", masked)
    line = text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "game must retain Dreamcast LF_FIELDLIST's compatible "
             "match_underground_gates-before-randomize_university order "
             "and NewmapCell* cell parameter; retail proves public access")]


def split_window_header_violations(text: str) -> list[tuple[int, str]]:
    """Audit the Dreamcast-proven TSplitWindow creature domain types."""
    masked = _source.mask(text)
    rules = (
        ("creature", "TSplitWindow::creature must retain TCreatureType",
         r"\bTCreatureType\s+creature\s*;"),
        ("TSplitWindow",
         "TSplitWindow constructor must take TCreatureType thisArmy",
         r"\bTSplitWindow\s*\(\s*int\s+\w+\s*,\s*int\s+\w+\s*,\s*"
         r"TCreatureType\s+\w+\s*\)\s*;"),
        ("splitSlider",
         "TSplitWindow::splitSlider must retain the canonical slider type",
         r"\bslider\s*\*\s*splitSlider\s*;"),
    )
    defects = []
    for token_name, description, pattern in rules:
        if re.search(pattern, masked, re.DOTALL) is not None:
            continue
        token = re.search(r"\b" + token_name + r"\b", masked)
        line = text.count("\n", 0, token.start()) + 1 if token else 1
        defects.append((line, description))
    return defects


def armygroup_has_some_undead_violations(
        header_text: str, source_text: str) -> list[tuple[int, str]]:
    """Audit armygrp.cpp:668's const member boundary and loop body."""
    header_masked = _source.mask(header_text)
    source_masked = _source.mask(source_text)
    defects: list[tuple[int, str]] = []
    header_pattern = (
        r"\bunsigned\s+char\s+HasSomeUndead\s*\(\s*\)\s*const\s*;")
    if re.search(header_pattern, header_masked) is None:
        token = re.search(r"\bHasSomeUndead\b", header_masked)
        line = (header_text.count("\n", 0, token.start()) + 1
                if token else 1)
        defects.append((
            line,
            "armygrp.cpp:668 HasSomeUndead must retain Dreamcast's public "
            "unsigned-char const member declaration"))
    source_pattern = (
        r"\bunsigned\s+char\s+armyGroup\s*::\s*HasSomeUndead\s*"
        r"\(\s*\)\s*const\s*\{\s*"
        r"for\s*\(\s*int\s+i\s*=\s*0\s*;\s*i\s*<\s*"
        r"ARMY_GROUP_SLOT_COUNT\s*;\s*\+\+i\s*\)\s*\{\s*"
        r"if\s*\(\s*armies\s*\[\s*i\s*\]\s*==\s*CREATURE_NONE\s*"
        r"\)\s*continue\s*;\s*if\s*\(\s*akCreatureTypeTraits\s*\["
        r"\s*armies\s*\[\s*i\s*\]\s*\]\s*\.\s*attributes\s*&\s*"
        r"CTA_UNDEAD\s*\)\s*return\s+1\s*;\s*\}\s*return\s+0\s*;"
        r"\s*\}")
    if re.search(source_pattern, source_masked, re.DOTALL) is None:
        token = re.search(r"\bHasSomeUndead\b", source_masked)
        line = (source_text.count("\n", 0, token.start()) + 1
                if token else 1)
        defects.append((
            line,
            "armygrp.cpp:668 HasSomeUndead must remain the Dreamcast-proven "
            "seven-slot member loop with CREATURE_NONE skip and undead test"))
    return defects


def armygroup_const_query_violations(
        header_text: str, source_text: str) -> list[tuple[int, str]]:
    """Audit the coherent Dreamcast-public const query surface."""
    header_masked = _source.mask(header_text)
    source_masked = _source.mask(source_text)
    defects: list[tuple[int, str]] = []
    names = (
        "HasCreatures", "HasAllUndead", "HasSomeUndead", "IsMember",
        "CanJoin", "GetAlignments", "get_AI_value", "GetNativeTerrain",
        "GetNumArmies", "GetMorale", "GetArmyMorale", "GetLuck",
        "GetArmyLuck", "get_morale_description", "get_luck_description",
    )

    def line_for(text: str, masked: str, name: str) -> int:
        token = re.search(r"\b" + re.escape(name) + r"\b", masked)
        return text.count("\n", 0, token.start()) + 1 if token else 1

    for name in names:
        escaped = re.escape(name)
        header_pattern = (
            r"\b" + escaped + r"\s*\([^;{}]*\)\s*const\s*;")
        if re.search(header_pattern, header_masked, re.DOTALL) is None:
            defects.append((
                line_for(header_text, header_masked, name),
                f"armyGroup::{name} must retain its Dreamcast-public QB "
                "const declaration"))
        source_pattern = (
            r"\barmyGroup\s*::\s*" + escaped
            + r"\s*\([^;{}]*\)\s*const\s*\{")
        if re.search(source_pattern, source_masked, re.DOTALL) is None:
            defects.append((
                line_for(source_text, source_masked, name),
                f"armyGroup::{name} must retain its Dreamcast-public QB "
                "const definition"))

    total_header = re.findall(
        r"\bget_creature_total\s*\(([^;{}]*)\)\s*const\s*;",
        header_masked, re.DOTALL)
    total_source = re.findall(
        r"\barmyGroup\s*::\s*get_creature_total\s*"
        r"\(([^;{}]*)\)\s*const\s*\{",
        source_masked, re.DOTALL)
    for text, masked, matches, location in (
            (header_text, header_masked, total_header, "declarations"),
            (source_text, source_masked, total_source, "definitions")):
        normalized = [re.sub(r"\s+", " ", value).strip()
                      for value in matches]
        if (len(normalized) != 2 or "" not in normalized
                or not any(re.search(r"\bTCreatureType\b", value)
                           for value in normalized)):
            defects.append((
                line_for(text, masked, "get_creature_total"),
                "both armyGroup::get_creature_total overloads must retain "
                f"their Dreamcast-public QB const {location}"))

    homogeneity_pattern = (
        r"\barmyGroup\s*::\s*GetHomogeneityMoraleAdjust\s*"
        r"\(\s*\)\s*const\s*\{")
    # The function is an explicitly retained DC_ONLY carcass.  Lexically
    # mask comments/strings without discarding its #if 0 evidence block.
    lexical_source = _source._mask_lex(source_text)
    if re.search(homogeneity_pattern, lexical_source) is None:
        defects.append((
            line_for(source_text, source_masked,
                     "GetHomogeneityMoraleAdjust"),
            "DC-only armyGroup::GetHomogeneityMoraleAdjust must retain its "
            "Dreamcast-public QB const definition"))
    return defects


def window_header_violations(text: str) -> list[tuple[int, str]]:
    """Audit the const-qualified Dreamcast findWidget helper pair."""
    masked = _source.mask(text)
    rules = (
        ("findWidget",
         "heroWindow::findWidget must retain its attested const qualifier",
         r"\bint\s+findWidget\s*\(\s*int\s+\w+\s*,\s*int\s+\w+\s*\)"
         r"\s*const\s*;"),
        ("findWidgetPtr",
         "heroWindow::findWidgetPtr must retain its attested const qualifier",
         r"\bwidget\s*\*\s*findWidgetPtr\s*\(\s*int\s+\w+\s*,\s*"
         r"int\s+\w+\s*\)\s*const\s*;"),
    )
    defects = []
    for token_name, description, pattern in rules:
        if re.search(pattern, masked, re.DOTALL) is not None:
            continue
        token = re.search(r"\b" + token_name + r"\b", masked)
        line = text.count("\n", 0, token.start()) + 1 if token else 1
        defects.append((line, description))
    return defects


def single_selection_window_contract_violations(
        header_text: str, source_text: str) -> list[tuple[int, str]]:
    """Audit the Dreamcast-proven CUpdatePlayerPosMsg source boundary.

    Constructor calls are implicit source forms and are deliberately outside
    the generic helper-token pass.  CodeView nevertheless proves this exact
    two-pointer constructor, its two named array members, the CNetMsg base
    construction, and the two memcpy statement rows.  Retail independently
    corroborates the layout and accepts this boundary byte-for-byte.
    """
    header = _source.mask(header_text)
    declaration = (
        r"\bCNetPlayerHandlerPlayer\s+m_netPlayer\s*\[\s*8\s*\]\s*;\s*"
        r"CNetPlayerHandlerPlayer\s+m_compPlayer\s*\[\s*8\s*\]\s*;.*?"
        r"\bCUpdatePlayerPosMsg\s*\(\s*CNetPlayerHandlerPlayer\s*\*\s*"
        r"pNetPlayers\s*,\s*CNetPlayerHandlerPlayer\s*\*\s*pCompPlayers"
        r"\s*\)\s*;")
    source = _source.mask(source_text)
    definition = (
        r"\b(?:inline\s+)?CUpdatePlayerPosMsg\s*::\s*"
        r"CUpdatePlayerPosMsg\s*\(\s*CNetPlayerHandlerPlayer\s*\*\s*"
        r"pNetPlayers\s*,\s*CNetPlayerHandlerPlayer\s*\*\s*pCompPlayers"
        r"\s*\)\s*:\s*CNetMsg\s*\(\s*RS_UPDATE_PLAYER_POS\s*,\s*"
        r"sizeof\s*\(\s*CUpdatePlayerPosMsg\s*\)\s*\)\s*\{\s*"
        r"memcpy\s*\(\s*m_netPlayer\s*,\s*pNetPlayers\s*,\s*sizeof\s*"
        r"\(\s*m_netPlayer\s*\)\s*\)\s*;\s*memcpy\s*\(\s*"
        r"m_compPlayer\s*,\s*pCompPlayers\s*,\s*sizeof\s*\(\s*"
        r"m_compPlayer\s*\)\s*\)\s*;\s*\}")
    if re.search(declaration, header, re.DOTALL) is not None \
            and re.search(definition, source, re.DOTALL) is not None:
        return []
    token = re.search(r"\bCUpdatePlayerPosMsg\b", header)
    line = header_text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "CUpdatePlayerPosMsg must retain Dreamcast's m_netPlayer and "
             "m_compPlayer arrays and two-pointer declaration in the header, "
             "with its cpp constructor boundary, CNetMsg base initializer, "
             "and ordered memcpy statements")]


def netplayer_constructor_contract_violations(
        header_text: str, source_text: str) -> list[tuple[int, str]]:
    """Audit the shared CNetPlayerInfo base-constructor boundary.

    Dreamcast CodeView records the base constructor in struct.h:340, with
    separate dpid and sName[0] statement rows, and records the derived
    CNetPlayerHandlerPlayer constructor calling that boundary before its own
    fields. Complete adds the version member to the retail base layout. The
    x86 constructor and all two-array expansions corroborate assigning that
    member in the base boundary before the derived heroIndex statement.
    """
    header = _source.mask(header_text)
    declaration = (
        r"\bclass\s+CNetPlayerInfo\s*\{.*?\bint\s+version\s*;.*?"
        r"\bCNetPlayerInfo\s*\(\s*\)\s*;")
    source = _source.mask(source_text)
    base_definition = (
        r"\binline\s+CNetPlayerInfo\s*::\s*CNetPlayerInfo\s*\(\s*\)"
        r"\s*\{\s*dpid\s*=\s*0\s*;\s*sName\s*\[\s*0\s*\]\s*=\s*"
        r"0\s*;\s*version\s*=\s*\*\s*gpVideoGameState\s*;\s*\}")
    derived_definition = (
        r"\bCNetPlayerHandlerPlayer\s*::\s*CNetPlayerHandlerPlayer\s*"
        r"\(\s*\)\s*\{\s*heroIndex\s*=\s*-\s*1\s*;\s*townIndex\s*="
        r"\s*-\s*1\s*;")
    if (re.search(declaration, header, re.DOTALL) is not None
            and re.search(base_definition, source, re.DOTALL) is not None
            and re.search(derived_definition, source, re.DOTALL) is not None):
        return []
    token = re.search(r"\bCNetPlayerInfo\b", header)
    line = header_text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "CNetPlayerInfo must retain Dreamcast's explicit default-"
             "constructor declaration and cpp boundary, with ordered dpid, "
             "sName[0], and Complete-only version initialization before the "
             "derived seat-record fields")]


def update_player_pos_signature_violations(
        header_text: str, source_text: str) -> list[tuple[int, str]]:
    """Audit OnUpdatePlayerPosMsg's shared void signature.

    Dreamcast CodeView records ``QAAXPAVCNetMsg@@@Z`` and a void procedure at
    line 7591.  Retail independently carries the same decorated name and has
    no return-value materialisation.  Bind the source check to the admitted
    retail VA so the disabled Dreamcast carcass cannot satisfy it.
    """
    header = _source.mask(header_text)
    source = _source.mask(source_text)
    declaration = (
        r"\bvoid\s+OnUpdatePlayerPosMsg\s*\(\s*CNetMsg\s*\*\s*"
        r"pNetMsg\s*\)\s*;")
    definition = (
        r"\bVA\s*\(\s*0x0058BA40\s*,\s*0x175\s*\).*?"
        r"\bvoid\s+TSingleSelectionWindow\s*::\s*OnUpdatePlayerPosMsg\s*"
        r"\(\s*CNetMsg\s*\*\s*pNetMsg\s*\)")
    if (re.search(declaration, header, re.DOTALL) is not None
            and re.search(definition, source, re.DOTALL) is not None):
        return []
    token = re.search(r"\bOnUpdatePlayerPosMsg\b", header)
    line = header_text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "OnUpdatePlayerPosMsg must retain Dreamcast's void declaration "
             "and the void retail definition at VA 0x0058BA40")]


def _helper_token(name: str) -> str | None:
    """Return the source-call identifier worth enforcing, or ``None``.

    Constructors, destructors and operators do not necessarily spell their
    CodeView procedure name at a call site (``new Foo`` and ``a[i]`` are the
    obvious cases), so this gate deliberately skips those implicit forms.
    Ordinary free functions are source-visible and are enforced too; compiler
    runtime names beginning with an underscore remain outside this pass.
    """
    scopes = dreamcast._top_level_scopes(name)
    if not scopes:
        return None
    if len(scopes) >= 2 and scopes[0].split("<", 1)[0] == "std":
        return None
    helper = scopes[-1].split("<", 1)[0]
    if len(scopes) >= 2:
        owner = scopes[-2].split("<", 1)[0]
        if helper == owner or helper == f"~{owner}" \
                or helper.startswith("operator"):
            return None
    if helper.startswith("_") or "$" in helper:
        return None
    return helper if re.fullmatch(r"[A-Za-z_]\w*", helper) else None


def _same_owner_call_present(active: str, helper: str, owner: str) -> bool:
    """Whether ``helper(...)`` names the current class rather than a peer.

    An unqualified call in a member body and an explicit ``this->``/class
    qualification preserve the current class's helper boundary.  A call on
    another explicit receiver does not: ``pDPlay->IsHost()`` cannot satisfy a
    Dreamcast edge to ``TSingleSelectionWindow::IsHost`` merely because the
    final identifier happens to agree.
    """
    pattern = re.compile(r"\b" + re.escape(helper) + r"\s*\(")
    for match in pattern.finditer(active):
        prefix = active[:match.start()]
        if re.search(r"\bthis\s*->\s*$", prefix):
            return True
        if re.search(r"(?:->|\.)\s*$", prefix):
            continue
        qualifier = re.search(
            r"([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*::\s*$", prefix)
        if qualifier is None:
            return True
        if qualifier.group(1).split("::")[-1] == owner:
            return True
    return False


def missing_from_body(body: str, callees: list[str],
                      key: tuple[str, int] | None = None) \
        -> list[tuple[str, str]]:
    """Return ``[(callee, helper)]`` not named in one active source body.

    Only decoded ``PROVEN_SELF_CALLS`` are receiver-aware.  Other edges retain
    the conservative identifier check because a same-class call can target a
    different instance and the lexical pass has no general receiver proof.
    """
    active = _source.mask(body)
    missing = []
    for callee in sorted(set(callees)):
        helper = _helper_token(callee)
        if helper is None:
            continue
        if key is not None and (key[0], key[1], callee) \
                in PROVEN_SELF_CALLS:
            callee_scopes = dreamcast._top_level_scopes(callee)
            callee_owner = callee_scopes[-2].split("<", 1)[0]
            present = _same_owner_call_present(
                active, helper, callee_owner)
        else:
            present = re.search(
                r"\b" + re.escape(helper) + r"\s*\(", active) is not None
        if not present:
            missing.append((callee, helper))
    return missing


def _statement_chunks(masked: str) -> list[str]:
    """Split a masked body at source-statement boundaries.

    Semicolons inside a ``for (...)`` header are not boundaries.  Braces and
    the end of a control header are boundaries so ``if (Ready()) Run();``
    remains two statement groups even when the body has no braces.
    """
    boundaries = {0, len(masked)}
    paren = bracket = 0
    for index, char in enumerate(masked):
        if char == "(":
            paren += 1
        elif char == ")" and paren:
            paren -= 1
        elif char == "[":
            bracket += 1
        elif char == "]" and bracket:
            bracket -= 1
        elif char == ";" and not paren and not bracket:
            boundaries.add(index + 1)
        elif char in "{}" and not paren and not bracket:
            boundaries.add(index + 1)

    for match in re.finditer(r"\b(?:if|for|while|switch|catch)\s*\(", masked):
        opening = masked.find("(", match.start(), match.end())
        closing = _source._match_paren(masked, opening, "(", ")")
        if closing is not None:
            boundaries.add(closing + 1)

    points = sorted(boundaries)
    return [masked[points[index]:points[index + 1]]
            for index in range(len(points) - 1)
            if masked[points[index]:points[index + 1]].strip()]


def misgrouped_from_body(body: str, groups: tuple[CallGroup, ...]) \
        -> CallGroup | None:
    """First DC call group that cannot map to ordered C++ statements.

    A CodeView breakpoint group may cover several adjacent source statements,
    so helpers in one group may map to the same or increasing C++ chunks.
    Their SH4 order must be preserved when they occupy different chunks.  If
    they share one chunk, argument evaluation and nesting may reverse lexical
    token order.  Distinct breakpoint groups introducing a helper must map to
    distinct, increasing chunks.  Repeated occurrences of an already-mapped
    helper are not an equal-call-count invariant: Complete may remove an
    older-revision statement while retaining the shared helper boundary
    elsewhere (the byte-exact SetupHeroView palette-message reduction is the
    motivating case).  Missing helpers are audited separately by
    ``missing_from_body``.
    """
    chunks = _statement_chunks(_source.mask(body))
    cursor = 0
    mapped_helpers: set[str] = set()
    for group in groups:
        helpers = [helper for callee in group.callees
                   if (helper := _helper_token(callee)) is not None]
        helpers = list(dict.fromkeys(
            helper for helper in helpers if helper not in mapped_helpers))
        if not helpers:
            continue
        counts = [Counter({
            helper: len(re.findall(
                r"\b" + re.escape(helper) + r"\s*\(", chunk))
            for helper in set(helpers)}) for chunk in chunks]
        used = [Counter() for _chunk in chunks]
        chunk_index = cursor
        last = None
        for helper in helpers:
            while (chunk_index < len(chunks)
                   and used[chunk_index][helper]
                       >= counts[chunk_index][helper]):
                chunk_index += 1
            if chunk_index >= len(chunks):
                return group
            used[chunk_index][helper] += 1
            last = chunk_index
        if last is None:
            return group
        mapped_helpers.update(helpers)
        cursor = last + 1
    return None


def contract_violations(body: str, key: tuple[str, int]) -> list[SourceRule]:
    """Selected no-call/order/nesting facts not expressible by DC xrefs."""
    active = _source.mask(body)
    directive_active = None
    defects = []
    for rule in SOURCE_RULES.get(key, ()):
        if rule.include_directives:
            if directive_active is None:
                directive_active = _source._mask_lex(body)
            haystack = directive_active
        else:
            haystack = active
        count = len(re.findall(rule.pattern, haystack, re.DOTALL))
        if count < rule.minimum \
                or rule.maximum is not None and count > rule.maximum:
            defects.append(rule)
    return defects


def transfer_satisfied(transfer: CallTransfer, caller_body: str,
                       receiver_body: str, exact_vas: set[int]) -> bool:
    """Whether one explicit cross-function source transfer still holds."""
    if transfer.receiver_va not in exact_vas:
        return False
    caller = _source.mask(caller_body)
    receiver = _source.mask(receiver_body)
    return re.search(transfer.caller_pattern, caller, re.DOTALL) is not None \
        and re.search(transfer.receiver_pattern, receiver, re.DOTALL) is not None


def apply_proven_call_spellings(
        key: tuple[str, int], body: str, va: int | None,
        exact_vas: set[int]) -> str:
    """Canonicalize bounded Complete call spellings for shape comparison."""
    if va is None or va not in exact_vas:
        return body
    active = _source.mask(body)
    canonical = body
    for spelling in PROVEN_CALL_SPELLINGS.get(key, ()):
        if va != spelling.caller_va \
                or re.search(spelling.retail_pattern, active) is None:
            continue
        canonical = re.sub(
            spelling.retail_pattern, spelling.canonical_name, canonical)
    return canonical


def proven_dc_only_order_helpers(
        key: tuple[str, int], body: str, va: int | None,
        exact_vas: set[int]) -> tuple[frozenset[str], tuple[str, ...]]:
    """Return bounded helper-order facts proved DC-only for this caller."""
    if va is None or va not in exact_vas:
        return frozenset(), ()
    active = _source.mask(body)
    helpers: set[str] = set()
    descriptions: list[str] = []
    for skew in PROVEN_ORDER_SKEWS.get(key, ()):
        if va != skew.caller_va \
                or re.search(skew.retail_pattern, active, re.DOTALL) is None:
            continue
        helpers.update(skew.dc_only_helpers)
        descriptions.append(skew.description)
    return frozenset(helpers), tuple(descriptions)


def groups_without_helpers(groups: tuple[CallGroup, ...],
                           omitted: frozenset[str]) \
        -> tuple[CallGroup, ...]:
    """Remove only explicitly classified helper tokens from call groups."""
    filtered = tuple(
        CallGroup(group.line, tuple(
            callee for callee in group.callees if callee not in omitted))
        for group in groups)
    return tuple(group for group in filtered if group.callees)


def groups_without_transfers(
        groups: tuple[CallGroup, ...], is_transferred) \
        -> tuple[CallGroup, ...]:
    """Remove only individually proved transferred calls from DC groups."""
    filtered = tuple(
        CallGroup(group.line, tuple(
            callee for callee in group.callees
            if not is_transferred(callee)))
        for group in groups)
    return tuple(group for group in filtered if group.callees)


def army_is_contract_violations(text: str, *, header: bool = False) \
        -> list[tuple[int, str]]:
    """Dreamcast's no-call Army.h bodies and constant-call contracts."""
    active = _source.mask(text)
    defects: list[tuple[int, str]] = []
    if header and len(re.findall(
            r"\b(?:sMonInfo\.)?creatureId\s*&\s*attribute\b", active)) < 2:
        defects.append((1, "army::Is must retain both mask-based header arms"))
    if header and not re.search(
            r"\bint\s+(?:army::)?get_owning_side\s*\(\s*\)\s*const\s*\{\s*"
            r"return\s+combatSide\s*;\s*\}", active, re.DOTALL):
        defects.append((1, "get_owning_side must retain its header inline"))
    if header:
        owning_match = re.search(r"int\s+(?:army::)?get_owning_side", text)
        owning = owning_match.start() if owning_match is not None else -1
        guard = text.rfind("#ifdef HOMM3_ARMY_AURA_VIEW", 0, owning)
        guard_end = text.find("#endif", guard) if guard >= 0 else -1
        if guard >= 0 and guard < owning < guard_end:
            defects.append((text.count("\n", 0, owning) + 1,
                            "get_owning_side may not be hidden by a TU view"))
    numeric = re.compile(
        r"\bIs\s*\(\s*(?:0x[0-9A-Fa-f]+|[0-9]+)\s*\)")
    for match in numeric.finditer(active):
        defects.append((text.count("\n", 0, match.start()) + 1,
                        "constant army::Is call passes a bit index, not a mask"))
    return defects


def _army_roster_spans(masked: str) \
        -> tuple[list[tuple[int, int, int, str]], list[tuple[int, str]]]:
    """Locate active Army.h header definitions as ``(line,start,end,name)``."""
    # Preserve offsets while removing the required out-of-class qualifier so
    # the concise signature table above can describe both names and types.
    searchable = masked.replace("army::", "      ")
    spans: list[tuple[int, int, int, str]] = []
    defects: list[tuple[int, str]] = []
    for source_line, name, signature in ARMY_HEADER_ROSTER:
        matches = list(re.finditer(r"inline\s+" + signature + r"\s*\{",
                                   searchable))
        if len(matches) != 1:
            defects.append((source_line,
                            f"Army.h:{source_line} {name} must have exactly "
                            "one active out-of-class header definition"))
            continue
        match = matches[0]
        head = masked[max(0, match.start() - 48):match.end()]
        if "army::" not in head:
            defects.append((source_line,
                            f"Army.h:{source_line} {name} must be defined "
                            "outside class army"))
            continue
        body_open = masked.rfind("{", match.start(), match.end())
        body_close = _source._match_paren(masked, body_open, "{", "}")
        if body_close is None:
            defects.append((source_line,
                            f"Army.h:{source_line} {name} has no closing brace"))
            continue
        spans.append((source_line, match.start(), body_close, name))
    return spans, defects


def army_header_roster_violations(text: str) -> list[tuple[int, str]]:
    """Audit the ordered, unconditional Army.h:718..881 definition run."""
    masked = _source.mask(text)
    spans, defects = _army_roster_spans(masked)
    if len(spans) != len(ARMY_HEADER_ROSTER):
        return defects

    positions = [start for _line, start, _end, _name in spans]
    if positions != sorted(positions):
        first = next(index for index in range(1, len(positions))
                     if positions[index] < positions[index - 1])
        line, _start, _end, name = spans[first]
        defects.append((line, f"Army.h inline roster is out of order at {name}"))

    by_line = {line: (start, end, name)
               for line, start, end, name in spans}
    for source_line, rules in ARMY_HEADER_BODY_RULES.items():
        start, end, name = by_line[source_line]
        body = masked[masked.find("{", start, end) + 1:end]
        for rule in rules:
            if len(re.findall(rule.pattern, body, re.DOTALL)) < rule.minimum:
                defects.append((source_line,
                                f"Army.h:{source_line} {name}: "
                                f"{rule.description}"))

    # LF_FIELDLIST proves these are after the class, not its tail: LeavesNoBody
    # is declared private at entry 234, is_in_area_highlight public at 203, yet
    # their bodies occur in the opposite source-line order here.  Require one
    # uninterrupted run after the army class and its size assertion.
    ordered = sorted(spans, key=lambda item: item[1])
    first_start = ordered[0][1]
    size_pattern = re.compile(r"SIZE\s*\(\s*army\s*,\s*0x548\s*\)\s*;")
    size = list(size_pattern.finditer(masked, 0, first_start))
    if not size or masked[size[-1].end():first_start].strip():
        defects.append((718, "Army.h inline definitions must follow the army class"))
    for index, (_line, _start, end, _name) in enumerate(ordered[:-1]):
        next_line, next_start, _next_end, next_name = ordered[index + 1]
        gap = masked[end + 1:next_start]
        if gap.strip():
            defects.append((next_line,
                            f"Army.h inline roster is not continuous before {next_name}"))
    # Preprocessor directives inside the two representation bridges are
    # compile scaffolding. Any directive around another roster member, or
    # around the block itself, recreates the forbidden TU-view source shape.
    block_start = size[-1].end() if size else first_start
    block_end = ordered[-1][2] + 1
    allowed = [by_line[718][:2], by_line[765][:2]]
    directive_pattern = re.compile(
        r"(?m)^[ \t]*#[ \t]*(?:if|ifdef|ifndef|elif|else|endif)\b")
    for match in directive_pattern.finditer(text, block_start, block_end):
        if not any(start <= match.start() <= end for start, end in allowed):
            defects.append((text.count("\n", 0, match.start()) + 1,
                            "Army.h inline roster may not be hidden by a TU view"))
    return defects


def army_class_roster_violations(text: str) -> list[tuple[int, str]]:
    """Audit declaration order/access from Dreamcast LF_FIELDLIST 0x205b.

    Retail-only additions are allowed between attested rows.  Shared rows may
    neither move nor disappear behind a TU view.  This deliberately does not
    consult the retail score: it is the source-fact gate that permits a
    temporary dip while the whole class declaration stream is reconstructed.
    """
    masked = _source.mask(text)
    class_match = re.search(r"\bclass\s+army\s*\{", masked)
    if class_match is None:
        return [(1, "Dreamcast army class declaration is missing")]
    class_open = masked.rfind("{", class_match.start(), class_match.end())
    class_close = _source._match_paren(masked, class_open, "{", "}")
    if class_close is None:
        return [(1, "army class declaration has no closing brace")]
    body_start = class_open + 1
    body = masked[body_start:class_close]
    defects: list[tuple[int, str]] = []

    for rule in ARMY_BOOL_DECLARATIONS:
        if len(re.findall(rule.pattern, body, re.DOTALL)) < rule.minimum:
            defects.append((text.count("\n", 0, body_start) + 1,
                            "LF_FIELDLIST/S_PUB32: " + rule.description))

    access_rows = [(0, "private")]
    access_rows.extend((match.end(), match.group(1)) for match in re.finditer(
        r"\b(public|private|protected)\s*:", body))
    access_points = [position for position, _access in access_rows]

    def line(position: int) -> int:
        return text.count("\n", 0, body_start + position) + 1

    def access_at(position: int) -> str:
        index = bisect.bisect_right(access_points, position) - 1
        return access_rows[index][1]

    def method_position(name: str) -> int | None:
        pattern = re.compile(r"(?<![:~\w])" + re.escape(name) + r"\s*\(")
        matches = list(pattern.finditer(body))
        return matches[0].start() if matches else None

    def field_position(name: str) -> int | None:
        # The word must be the declarator immediately before an optional array
        # suffix and semicolon, not a parameter or a use in another name.
        pattern = re.compile(r"\b" + re.escape(name)
                             + r"\b\s*(?:\[[^\]]+\]\s*)?;")
        match = pattern.search(body)
        return match.start() if match else None

    ordered: list[tuple[int, str]] = []
    for expected_access, names in (
            ("public", ARMY_PUBLIC_METHOD_ORDER),
            ("private", ARMY_PRIVATE_METHOD_ORDER)):
        for name in names:
            position = method_position(name)
            if position is None:
                defects.append((1, f"LF_FIELDLIST army::{name} declaration "
                                   "is missing or hidden by a TU view"))
                continue
            ordered.append((position, name))
            actual_access = access_at(position)
            if actual_access != expected_access:
                defects.append((line(position),
                                f"LF_FIELDLIST army::{name} is "
                                f"{expected_access}, not {actual_access}"))

    last_position = -1
    last_name = "public data"
    for position, name in ordered:
        if position <= last_position:
            defects.append((line(position),
                            f"LF_FIELDLIST order requires army::{name} "
                            f"after army::{last_name}"))
        else:
            last_position = position
            last_name = name

    anchor = field_position("backlashChance")
    constructor = method_position("army")
    if anchor is None:
        defects.append((1, "final reconstructed public field backlashChance "
                           "is missing"))
    elif constructor is not None and constructor <= anchor:
        defects.append((line(constructor),
                        "LF_FIELDLIST methods must follow the public data run"))

    tail_positions: list[tuple[int, str]] = []
    for dc_name, source_name in ARMY_PRIVATE_TAIL:
        position = field_position(source_name)
        if position is None:
            defects.append((1, f"LF_FIELDLIST private field {dc_name} "
                               f"({source_name}) is missing or hidden"))
            continue
        tail_positions.append((position, dc_name))
        actual_access = access_at(position)
        if actual_access != "private":
            defects.append((line(position),
                            f"LF_FIELDLIST field {dc_name} is private, "
                            f"not {actual_access}"))
    previous = last_position
    previous_name = last_name
    for position, name in tail_positions:
        if position <= previous:
            defects.append((line(position),
                            f"LF_FIELDLIST order requires field {name} "
                            f"after {previous_name}"))
        else:
            previous = position
            previous_name = name

    destructor = re.search(r"~army\s*\(", body)
    if destructor is not None:
        defects.append((line(destructor.start()),
                        "LF_FIELDLIST marks ~army compgen; do not declare it "
                        "as an explicit source member"))
    return defects


def selftest() -> list[str]:
    """Negative controls: the gate must detect the real defect class."""
    failures = []
    frozen_call = MissingCall(
        0x00401230, "sample.obj", 0x1230, "src/sample.cpp", 10,
        "Caller", "hero::HasSecondarySkill", "HasSecondarySkill")
    moved_call = MissingCall(
        None, "sample.obj", 0x1230, "src/renamed.cpp", 900,
        "PromotedCaller", "hero::HasSecondarySkill", "HasSecondarySkill")
    replacement_debt = MissingCall(
        0x00401230, "sample.obj", 0x1230, "src/sample.cpp", 10,
        "Caller", "hero::GetHeroSpellBonus", "GetHeroSpellBonus")
    frozen = {violation_key(frozen_call)}
    if violation_key(frozen_call) != violation_key(moved_call):
        failures.append("ratchet identity changed with source location/label")
    if new_violations([moved_call], frozen):
        failures.append("frozen caller/callee edge was treated as new")
    if new_violations([replacement_debt], frozen) != [replacement_debt]:
        failures.append("new flattened helper edge was not fatal")
    # Restoring one frozen edge cannot pay for flattening another: identities,
    # not the aggregate omission count, are the ratchet currency.
    if not new_violations([replacement_debt], frozen):
        failures.append("equal-count source-shape debt trade passed")
    if ratcheted_backlog([], frozen):
        failures.append("retired source-shape debt did not ratchet down")
    if ratcheted_backlog([replacement_debt], frozen):
        failures.append("fresh debt delayed retirement or entered baseline")
    audited = {_key_audit_scope(violation_key(frozen_call))}
    if ratcheted_backlog([replacement_debt], frozen, audited):
        failures.append("audited restoration was not banked on a red gate")
    if ratcheted_backlog([], frozen, set()) != frozen:
        failures.append("unaudited debt was retired on a red gate")
    file_contract = FileContractViolation(
        "include/game.h", 10, "GetHero keeps its helper boundary")
    moved_contract = FileContractViolation(
        "include/game.h", 800, "GetHero keeps its helper boundary")
    if violation_key(file_contract) != violation_key(moved_contract):
        failures.append("file-contract ratchet identity depended on line")
    attested = ["hero::HasSecondarySkill"]
    direct = "if (currentHero->skillOrder[eSecSkillBattleTactics] > 0) {}"
    if missing_from_body(direct, attested) != [
            ("hero::HasSecondarySkill", "HasSecondarySkill")]:
        failures.append("direct skillOrder test hid missing HasSecondarySkill")
    aligned = "if (currentHero->HasSecondarySkill(eSecSkillBattleTactics)) {}"
    if missing_from_body(aligned, attested):
        failures.append("attested HasSecondarySkill call did not pass")
    host_edge = ["TSingleSelectionWindow::IsHost"]
    host_key = ("singleselectionwindow.obj", 0x136388)
    wrong_host = "if (pDPlay->IsHost()) SendPlayerPositions(0);"
    if missing_from_body(wrong_host, host_edge, host_key) != [
            ("TSingleSelectionWindow::IsHost", "IsHost")]:
        failures.append("namesake receiver hid proven self-call IsHost")
    if missing_from_body(
            "if (IsHost()) SendPlayerPositions(0);", host_edge,
            host_key):
        failures.append("unqualified proven self-call IsHost did not pass")
    if missing_from_body(
            "if (this->IsHost()) SendPlayerPositions(0);", host_edge,
            host_key):
        failures.append("this-qualified proven self-call IsHost did not pass")
    if missing_from_body(wrong_host, host_edge):
        failures.append("unproved peer receiver became globally fatal")
    setup_probe = """\
if (mapChanged) {
    for (int i = 0; i < CNetPlayerHandler::MAX_PLAYERS; ++i) {}
}
int nextColor = 0;
widget* playerName;
UpdateGameVars();
int i;
for (i = 0; i < CNetPlayerHandler::MAX_PLAYERS; ++i) {
    int strNbr = 0;
}
"""
    if contract_violations(setup_probe, host_key):
        failures.append("aligned SetupAdvancedOptions local scopes did not pass")
    setup_mutations = (
        (setup_probe.replace("for (int i = 0;", "for (i = 0;", 1),
         "reset-loop i"),
        (setup_probe.replace("int i;\nfor (i = 0;", "for (int i = 0;"),
         "function-scope"),
        (setup_probe.replace("int strNbr = 0;", "int strNbr;"),
         "strNbr"),
    )
    for probe, description in setup_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, host_key)):
            failures.append("broken SetupAdvancedOptions " + description
                            + " source shape passed")
    current_map_key = ("singleselectionwindow.obj", 0x13BC60)
    current_map_probe = """\
if (map >= static_cast<int>(GetMapCount()))
    return;
message msg;
if (bUpdate) {
    if (mapChanged) {
        DrawWindow(0, 0xffff0001, 0xffff);
    } else {
        widget* temp = GetWidget(101);
        if (temp->status & widget::WIDGET_ACTIVE) {
            temp->Draw();
        }
    }
}
if (m_flag64) {
    for (int i = 0; i < CNetPlayerHandler::MAX_PLAYERS; ++i) {
        CNetPlayerHandlerPlayer* player = m_players.GetPlayerInPos(i);
        if (!player)
            player = m_players.GetCompPlayerInPos(i);
        player->handicap = gpGame->setup.handicap[i];
        widget* w = GetWidget(207 + i);
        if (w) {
            int saveStatus = w->status;
        }
    }
}
"""
    if contract_violations(current_map_probe, current_map_key):
        failures.append("aligned SetCurrentMap local scopes did not pass")
    current_map_mutations = (
        (current_map_probe.replace("message msg;", "message tempMsg;"),
         "function-scope msg"),
        (current_map_probe.replace("widget* temp = GetWidget(101);",
                                   "widget* w = GetWidget(101);"),
         "bUpdate-scope widget"),
        (current_map_probe.replace("for (int i = 0;", "for (int pos = 0;"),
         "second i"),
        (current_map_probe.replace("CNetPlayerHandlerPlayer* player =",
                                   "CNetPlayerHandlerPlayer* p ="),
         "player"),
        (current_map_probe.replace("int saveStatus = w->status;",
                                   "int saved = w->status;"),
         "saveStatus"),
    )
    for probe, description in current_map_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, current_map_key)):
            failures.append("broken SetCurrentMap " + description
                            + " source shape passed")
    palette_cycle_key = ("palette.obj", 0x10AA98)
    palette_cycle_probe = """\
if (step > 0) {
    for (int i = 0; i < step; ++i) {
        unsigned short saved = data[begin];
        memmove(&data[begin], &data[begin + 1],
                (end - begin) * sizeof(data[0]));
        data[end] = saved;
    }
} else {
    for (int i = 0; i < -step; ++i) {
        unsigned short saved = data[end];
        memmove(&data[begin + 1], &data[begin],
                (end - begin) * sizeof(data[0]));
        data[begin] = saved;
    }
}
"""
    if contract_violations(palette_cycle_probe, palette_cycle_key):
        failures.append("aligned TPalette16::Cycle source shape did not pass")
    palette_cycle_mutations = (
        (palette_cycle_probe.replace("i < step", "i < end - begin", 1),
         "positive-step loop"),
        (palette_cycle_probe.replace(
            "memmove(&data[begin], &data[begin + 1]",
            "memmove(&data[begin + 1], &data[begin]", 1),
         "left memmove"),
        (palette_cycle_probe.replace("i < -step", "i < step"),
         "negative-step loop"),
        (palette_cycle_probe.replace(
            "unsigned short saved = data[end];",
            "unsigned short saved = data[begin];"),
         "saved end endpoint"),
    )
    for probe, description in palette_cycle_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, palette_cycle_key)):
            failures.append("broken TPalette16::Cycle " + description
                            + " source shape passed")
    palette_gray_key = ("palette.obj", 0x10B7AC)
    palette_gray_probe = """\
const unsigned int red_norm =
    std::numeric_limits<int>::max() / red_mask;
const unsigned int green_norm =
    std::numeric_limits<int>::max() / green_mask;
const unsigned int blue_norm =
    std::numeric_limits<int>::max() / blue_mask;
for (int i = 10; i < 256; ++i) {
    unsigned int red = (data[i] & red_mask) * red_norm;
    unsigned int green = (data[i] & green_mask) * green_norm;
    unsigned int blue = (data[i] & blue_mask) * blue_norm;
    unsigned int gray = max(max(red, green), blue);
    data[i] = static_cast<unsigned short>(
        ((gray / red_norm) & red_mask) |
        ((gray / green_norm) & green_mask) |
        ((gray / blue_norm) & blue_mask));
}
"""
    if contract_violations(palette_gray_probe, palette_gray_key):
        failures.append("aligned TPalette16::Gray source shape did not pass")
    palette_gray_mutations = (
        (palette_gray_probe.replace(
            "const unsigned int red_norm =\n"
            "    std::numeric_limits<int>::max() / red_mask;\n"
            "const unsigned int green_norm =\n"
            "    std::numeric_limits<int>::max() / green_mask;",
            "const unsigned int green_norm =\n"
            "    std::numeric_limits<int>::max() / green_mask;\n"
            "const unsigned int red_norm =\n"
            "    std::numeric_limits<int>::max() / red_mask;"),
         "normalization locals"),
        (palette_gray_probe.replace("int i = 10", "int i = 0"),
         "entry-10 loop"),
        (palette_gray_probe.replace(
            "max(max(red, green), blue)",
            "red > green ? (red > blue ? red : blue) : "
            "(green > blue ? green : blue)"),
         "nested max"),
        (palette_gray_probe.replace(
            "((gray / blue_norm) & blue_mask)",
            "((gray / blue_norm) & red_mask)"),
         "packed write"),
    )
    for probe, description in palette_gray_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, palette_gray_key)):
            failures.append("broken TPalette16::Gray " + description
                            + " source shape passed")
    send_key = ("singleselectionwindow.obj", 0x140D74)
    send_probe = """\
CUpdatePlayerPosMsg msg(m_players.humanPlayers,
                        m_players.computerPlayers);
TransmitRemoteDataDPID(&msg, dpidTo, true, true);
return 1;
"""
    if contract_violations(send_probe, send_key):
        failures.append("aligned SendPlayerPositions constructor did not pass")
    flattened_send = """\
CUpdatePlayerPosMsg msg;
memcpy(msg.m_netPlayer, m_players.humanPlayers, sizeof(msg.m_netPlayer));
memcpy(msg.m_compPlayer, m_players.computerPlayers, sizeof(msg.m_compPlayer));
TransmitRemoteDataDPID(&msg, dpidTo, true, true);
return 1;
"""
    if not contract_violations(flattened_send, send_key):
        failures.append("flattened SendPlayerPositions constructor passed")
    choose_town_key = ("singleselectionwindow.obj", 0x143214)
    choose_town_probe = """\
CMapHeaderData* mp = &gpGame->mapHeader;
CMapHeaderData::TPlayerSlotAttributes* slotAtt =
    &mp->playerSlotAttributes[gamePos];
CNetPlayerHandlerPlayer* player = m_players.GetPlayerInPos(gamePos);
unsigned char isHotSeat = mode == 3;
if (player->IsHuman() && player->dpid != localDpid) return 0;
if (!player->IsHuman() && !IsHost()) return 0;
"""
    if contract_violations(choose_town_probe, choose_town_key):
        failures.append("aligned CanChooseTown helper shape did not pass")
    broken_choose_town_probes = (
        choose_town_probe.replace(
            "if (!player->IsHuman() && !IsHost()) return 0;", ""),
        choose_town_probe.replace("!IsHost()", "!pDPlay->IsHost()"),
        choose_town_probe.replace(
            "CNetPlayerHandlerPlayer* player = m_players.GetPlayerInPos(gamePos);\n"
            "unsigned char isHotSeat = mode == 3;",
            "unsigned char isHotSeat = mode == 3;\n"
            "CNetPlayerHandlerPlayer* player = m_players.GetPlayerInPos(gamePos);"),
    )
    if any(not contract_violations(probe, choose_town_key)
           for probe in broken_choose_town_probes):
        failures.append("broken CanChooseTown helper shape passed")
    choose_hero_key = ("singleselectionwindow.obj", 0x14332C)
    choose_hero_probe = choose_town_probe + """\
if (GetDisplayTown(gamePos) == -1) return 0;
"""
    if contract_violations(choose_hero_probe, choose_hero_key):
        failures.append("aligned CanChooseHero helper shape did not pass")
    broken_choose_hero_probes = (
        choose_hero_probe.replace(
            "if (!player->IsHuman() && !IsHost()) return 0;", ""),
        choose_hero_probe.replace(
            "if (GetDisplayTown(gamePos) == -1) return 0;",
            "int town = player->townIndex;\nif (town == -1) return 0;"),
        choose_hero_probe.replace(
            "CNetPlayerHandlerPlayer* player = m_players.GetPlayerInPos(gamePos);\n"
            "unsigned char isHotSeat = mode == 3;",
            "unsigned char isHotSeat = mode == 3;\n"
            "CNetPlayerHandlerPlayer* player = m_players.GetPlayerInPos(gamePos);"),
    )
    if any(not contract_violations(probe, choose_hero_key)
           for probe in broken_choose_hero_probes):
        failures.append("broken CanChooseHero helper shape passed")
    transmit_key = ("singleselectionwindow.obj", 0x1406EC)
    transmit_probe = """\
int iMonthType = giMonthType;
int iMonthTypeExtra = giMonthTypeExtra;
int iWeekType = giWeekType;
int iWeekTypeExtra = giWeekTypeExtra;
giMonthType = iMonthType;
giMonthTypeExtra = iMonthTypeExtra;
giWeekType = iWeekType;
giWeekTypeExtra = iWeekTypeExtra;
gUnnamed69778c = gLocalGamePos;
gMapVisibilityBit = 1 << gLocalGamePos;
gUnnamed69d810 = gNetLocalGamePos;
UpdateTurnDuration();
"""
    if contract_violations(transmit_probe, transmit_key):
        failures.append("aligned OnGameTransmitInitMsg order did not pass")
    transmit_mutations = (
        transmit_probe.replace(
            "int iMonthType = giMonthType;\n"
            "int iMonthTypeExtra = giMonthTypeExtra;",
            "int iMonthTypeExtra = giMonthTypeExtra;\n"
            "int iMonthType = giMonthType;"),
        transmit_probe.replace(
            "gUnnamed69778c = gLocalGamePos;\n"
            "gMapVisibilityBit = 1 << gLocalGamePos;\n"
            "gUnnamed69d810 = gNetLocalGamePos;",
            "gUnnamed69d810 = gNetLocalGamePos;\n"
            "gUnnamed69778c = gLocalGamePos;\n"
            "gMapVisibilityBit = 1 << gLocalGamePos;"),
    )
    if any(not contract_violations(probe, transmit_key)
           for probe in transmit_mutations):
        failures.append("reordered OnGameTransmitInitMsg source shape passed")
    bottom_town_key = ("bottomviewsubwindow.obj", 0x55DF4)
    bottom_town_probe = """\
if (which->HasBuilding(HALL_TOWN_ID, 0)) hallLevel = 1;
else if (which->HasBuilding(HALL_CITY_ID, 0)) hallLevel = 2;
else if (which->HasBuilding(HALL_CAPITOL_ID, 0)) hallLevel = 3;
std::string town_size_name = gTownSizeNames[hallLevel];
if (which->HasBuilding(CASTLE_FORT_ID, 0)) fortLevel = 0;
else if (which->HasBuilding(CASTLE_CITADEL_ID, 0)) fortLevel = 1;
else if (which->HasBuilding(CASTLE_CASTLE_ID, 0)) fortLevel = 2;
if (which->HasBuilding(MARKETPLACE_SILO_ID, 1)) {
    int* resource = which->get_silo_income();
    for (int i = 0; i <= 6; ++i) {
        if (resource[i] != 0) slots[found++] = i;
    }
}
"""
    if contract_violations(bottom_town_probe, bottom_town_key):
        failures.append("aligned TBottomViewTown source shape did not pass")
    bottom_town_mutations = (
        bottom_town_probe.replace(
            "which->HasBuilding(MARKETPLACE_SILO_ID, 1)",
            "which->active & bitNumber[MARKETPLACE_SILO_ID]"),
        bottom_town_probe.replace(
            "HasBuilding(MARKETPLACE_SILO_ID, 1)",
            "HasBuilding(MARKETPLACE_SILO_ID, 0)"),
        bottom_town_probe.replace("int* resource", "int* income").replace(
            "resource[i]", "income[i]"),
        bottom_town_probe.replace(
            "if (which->HasBuilding(MARKETPLACE_SILO_ID, 1)) {\n"
            "    int* resource = which->get_silo_income();",
            "int* resource = which->get_silo_income();\n"
            "if (which->HasBuilding(MARKETPLACE_SILO_ID, 1)) {"),
        bottom_town_probe.replace(
            "std::string town_size_name = gTownSizeNames[hallLevel];\n", ""),
        bottom_town_probe.replace(
            "std::string town_size_name = gTownSizeNames[hallLevel];",
            "if (hallLevel) {\n"
            "    std::string town_size_name = gTownSizeNames[hallLevel];\n"
            "}"),
    )
    if any(not contract_violations(probe, bottom_town_key)
           for probe in bottom_town_mutations):
        failures.append("flattened TBottomViewTown source fact passed")
    sacrifice_key = ("sacrifice_window.obj", 0x125E08)
    sacrifice_probe = """\
if (!creature->field_04) {
    result = convert_with_commas(creature->amount);
} else {
    result = convert_with_commas(available);
}
"""
    if contract_violations(sacrifice_probe, sacrifice_key):
        failures.append("aligned update_creature_offering scopes did not pass")
    flattened_sacrifice = """\
if (!creature->field_04)
    result = convert_with_commas(creature->amount);
else
    result = convert_with_commas(available);
"""
    if not contract_violations(flattened_sacrifice, sacrifice_key):
        failures.append("flattened update_creature_offering scopes passed")
    commented = "// currentHero->HasSecondarySkill(0);\nif (flag) {}"
    if not missing_from_body(commented, attested):
        failures.append("commented helper call was treated as source shape")
    if missing_from_body("Foo value;", ["Foo::Foo", "Foo::~Foo",
                                         "Foo::operator[]"]):
        failures.append("implicit constructor/destructor/operator was enforced")
    if missing_from_body("", ["std::vector<int>::_M_insert_overflow"]):
        failures.append("compiler/library implementation helper was enforced")
    first_aid_key = ("command.obj", 0x6F824)
    first_aid_probe = """\
format_string(currentArmy->GetName(), targetArmy->GetName());
"""
    if contract_violations(first_aid_probe, first_aid_key):
        failures.append("aligned process_first_aid GetName shape did not pass")
    first_aid_mutations = (
        first_aid_probe.replace("targetArmy->GetName()", "target_name"),
        first_aid_probe + "akCreatureTypeTraits[type].m_name;\n",
        "#pragma inline_depth(0)\n" + first_aid_probe
        + "#pragma inline_depth()\n",
    )
    if any(not contract_violations(probe, first_aid_key)
           for probe in first_aid_mutations):
        failures.append("de-inlined process_first_aid GetName shape passed")
    process_next_key = ("command.obj", 0x6F984)
    process_next_probe = """\
message = format_string(currentArmy->GetName());
message = format_string(currentArmy->GetName());
message = format_string(currentArmy->GetName());
message = format_string(currentArmy->GetName());
"""
    if contract_violations(process_next_probe, process_next_key):
        failures.append("aligned ProcessNextAction GetName shape did not pass")
    process_next_mutations = (
        process_next_probe.replace(
            "message = format_string(currentArmy->GetName());", "", 1),
        process_next_probe + "akCreatureTypeTraits[type].m_plural_name;\n",
        "#pragma inline_depth(0)\n" + process_next_probe
        + "#pragma inline_depth()\n",
    )
    if any(not contract_violations(probe, process_next_key)
           for probe in process_next_mutations):
        failures.append("de-inlined ProcessNextAction GetName shape passed")
    refs = [XrefCall(0x100, "RealPoolCall", 1, 0),
            XrefCall(0x200, "DataAddressCollision", 2, 0),
            XrefCall(0x300, "DirectBsrCall", 0, 1)]
    if _attested_names(refs, {0x100}) != {
            "RealPoolCall", "DirectBsrCall"}:
        failures.append("literal-pool data reference was treated as a call")
    provenance_probe = """\
#if 0
void army::CheckLuck() { get_controller(); }
#endif
void army::CheckLuck() { iLuckStatus = 0; }
"""
    probe_mask = _source.mask(provenance_probe)
    definitions = _definitions_between(
        probe_mask, "army::CheckLuck", 0, len(probe_mask))
    if len(definitions) != 1:
        failures.append("active unclaimed helper definition was not isolated")
    elif not missing_from_body(
            provenance_probe[definitions[0].body_open + 1:
                             definitions[0].body_close],
            ["army::get_controller"]):
        failures.append("unclaimed helper could flatten an attested child")
    header_probe = """\
// E:\\gamedcs\\Army.h:810
const char* GetName() const { return GetArmyName(creatureType, numTroops); }
// E:\\gamedcs\\Army.h:815
const char* GetName(int count) const;
"""
    header_mask = _source.mask(header_probe)
    header_marks = list(PROVENANCE_RE.finditer(header_probe))
    first = _definitions_between(
        header_mask, "army::GetName", header_marks[0].end(),
        header_marks[1].start())
    second = _definitions_between(
        header_mask, "army::GetName", header_marks[1].end(),
        len(header_probe))
    if len(first) != 1 or missing_from_body(
            header_probe[first[0].body_open + 1:first[0].body_close],
            ["GetArmyName"]):
        failures.append("active header helper definition was not audited")
    if second:
        failures.append("header declaration was treated as a definition")
    if missing_from_body("return traits[type].name;", ["GetArmyName"]) != [
            ("GetArmyName", "GetArmyName")]:
        failures.append("flattened free helper call was not detected")
    transfer = PROVEN_CALL_TRANSFERS[
        ("adventureoptionswindow.obj", 0x5204, "game::ShowScenInfo")]
    transfer_caller = """\
gpWindowManager->dialogReturn = msg->codeY;
msg->codeY = widget::WIDGET_END_DIALOG;
msg->codeX = widget::WIDGET_END_DIALOG;
return MESSAGE_DISPATCH_FORWARD;
"""
    transfer_receiver = """\
switch (gpWindowManager->dialogReturn) {
case TAdventureOptionsWindow::VIEW_SCENARIO_ID:
    gpGame->ShowScenInfo();
    break;
}
"""
    if not transfer_satisfied(transfer, transfer_caller, transfer_receiver,
                              {transfer.receiver_va}):
        failures.append("proven cross-function call transfer did not pass")
    if transfer_satisfied(transfer, transfer_caller, transfer_receiver, set()):
        failures.append("non-exact transfer receiver passed")
    if transfer_satisfied(
            transfer, transfer_caller,
            transfer_receiver.replace("gpGame->ShowScenInfo();", ""),
            {transfer.receiver_va}):
        failures.append("transfer with erased receiver helper passed")
    if transfer_satisfied(
            transfer,
            transfer_caller.replace(
                "gpWindowManager->dialogReturn = msg->codeY;", ""),
            transfer_receiver, {transfer.receiver_va}):
        failures.append("transfer with erased caller forwarding passed")
    transfer_groups = (CallGroup(187, ("game::ShowScenInfo",)),
                       CallGroup(211, ("heroWindow::findWidget",)))
    filtered_groups = groups_without_transfers(
        transfer_groups, lambda callee: callee == "game::ShowScenInfo")
    if filtered_groups != (CallGroup(
            211, ("heroWindow::findWidget",)),):
        failures.append("proved transfer did not leave other call groups intact")
    if groups_without_transfers(transfer_groups, lambda _callee: False) \
            != transfer_groups:
        failures.append("unproved transfer disappeared from call groups")
    spelling_key = ("game.obj", 0xA99D0)
    spelling_va = PROVEN_CALL_SPELLINGS[spelling_key][0].caller_va
    spelling_body = "return _strnicmp(left, right, 8);"
    canonical = apply_proven_call_spellings(
        spelling_key, spelling_body, spelling_va, {spelling_va})
    if missing_from_body(canonical, ["strnicmp"]):
        failures.append("exact Complete call spelling did not canonicalize")
    unproved = apply_proven_call_spellings(
        spelling_key, spelling_body, spelling_va, set())
    if not missing_from_body(unproved, ["strnicmp"]):
        failures.append("non-exact call spelling bypassed source-shape gate")
    order_key = ("spells.obj", 0x152DEC)
    order_va = PROVEN_ORDER_SKEWS[order_key][0].caller_va
    complete_order = """\
switch (spell) {
case SPELL_RESURRECTION:
    return find_resurrection_target(side, hex, creature_spell);
case SPELL_ANIMATE_DEAD:
    return find_animate_dead_target(side, hex);
case SPELL_SACRIFICE:
    if (first_target)
        return find_resurrection_target(side, hex, creature_spell);
    break;
}
return cells[hex].get_army();
"""
    order_helpers, order_descriptions = proven_dc_only_order_helpers(
        order_key, complete_order, order_va, {order_va})
    if order_helpers != frozenset(
            PROVEN_ORDER_SKEWS[order_key][0].dc_only_helpers) \
            or len(order_descriptions) != 1:
        failures.append("exact Complete helper-order skew did not classify")
    if proven_dc_only_order_helpers(
            order_key, complete_order, order_va, set())[0]:
        failures.append("non-exact helper-order skew classified DC-only")
    dreamcast_order = complete_order.replace(
        "case SPELL_RESURRECTION:\n"
        "    return find_resurrection_target(side, hex, creature_spell);\n"
        "case SPELL_ANIMATE_DEAD:\n"
        "    return find_animate_dead_target(side, hex);\n"
        "case SPELL_SACRIFICE:",
        "case SPELL_SACRIFICE:")
    if proven_dc_only_order_helpers(
            order_key, dreamcast_order, order_va, {order_va})[0]:
        failures.append("unproved helper order classified DC-only")
    order_groups = (
        CallGroup(2606, ("combatManager::ValidHex",)),
        CallGroup(2615, ("combatManager::find_resurrection_target",)),
        CallGroup(2617, ("hexcell::get_army",)),
        CallGroup(2625, ("combatManager::find_animate_dead_target",)),
    )
    if groups_without_helpers(order_groups, order_helpers) != (
            CallGroup(2606, ("combatManager::ValidHex",)),):
        failures.append("DC-only order filter erased an unrelated helper")
    check_do_main_key = ("philai.obj", 0x10D47C)
    check_do_main_probe = """\
if (!(gUnnamed69cca4 & 1)) {
    gUnnamed69cca4 |= 1;
    iLastFrameRateTimer = GameTime::Get();
}
if (GameTime::ElapsedSince(iLastFrameRateTimer) > 15
    || GameTime::IsPast(glTimers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT])) {
    Process1WindowsMessage();
    PollSound();
    if (GameTime::IsPast(
            glTimers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT])) {
        if (!bMouseOnly)
            bSpecialHideCursor = 0;
        glTimers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] =
            GameTime::Get() + 180;
    }
    iLastFrameRateTimer = GameTime::Get();
}
"""
    if contract_violations(check_do_main_probe, check_do_main_key):
        failures.append("aligned CheckDoMain source shape did not pass")
    reordered_check_do_main = check_do_main_probe.replace(
        "    Process1WindowsMessage();\n    PollSound();",
        "    PollSound();\n    Process1WindowsMessage();")
    if not any("Process1WindowsMessage then PollSound" in rule.description
               for rule in contract_violations(
                   reordered_check_do_main, check_do_main_key)):
        failures.append("reordered CheckDoMain pump helpers passed")
    flattened_check_do_main = check_do_main_probe.replace(
        "GameTime::ElapsedSince(iLastFrameRateTimer) > 15",
        "static_cast<long>(GameTime::Get() - iLastFrameRateTimer) > 15")
    if not any("elapsed-or-animation-deadline" in rule.description
               for rule in contract_violations(
                   flattened_check_do_main, check_do_main_key)):
        failures.append("flattened CheckDoMain ElapsedSince boundary passed")
    wrong_mouse_gate = check_do_main_probe.replace(
        "if (!bMouseOnly)\n            bSpecialHideCursor = 0;",
        "if (!bForceMouseCheck)\n            bSpecialHideCursor = 0;")
    if not any("bMouseOnly cursor clear" in rule.description
               for rule in contract_violations(
                   wrong_mouse_gate, check_do_main_key)):
        failures.append("wrong CheckDoMain mouse-gate parameter passed")
    war_factory_key = ("philai.obj", 0x10DEA8)
    war_factory_probe = """\
TCreatureType creature = siege_artifact_to_creature(engine);
const double* resource_values = gpCurrentPlayer->resourceValue;
const int* costs = akCreatureTypeTraits[creature].cost;
long resource_cost = 0;
"""
    if contract_violations(war_factory_probe, war_factory_key):
        failures.append("aligned value_of_war_factory contract did not pass")
    raw_war_factory = war_factory_probe.replace(
        "const int* costs = akCreatureTypeTraits[creature].cost;",
        "const int* costs = gCreatureRecords + creature * 29 + 8;")
    raw_war_factory_rules = contract_violations(
        raw_war_factory, war_factory_key)
    if not any("raw record view" in rule.description
               for rule in raw_war_factory_rules):
        failures.append("raw value_of_war_factory cost row passed")
    reordered_war_factory = war_factory_probe.replace(
        "const int* costs = akCreatureTypeTraits[creature].cost;\n"
        "long resource_cost = 0;",
        "long resource_cost = 0;\n"
        "const int* costs = akCreatureTypeTraits[creature].cost;")
    if not any("before the resource accumulator" in rule.description
               for rule in contract_violations(
                   reordered_war_factory, war_factory_key)):
        failures.append("reordered value_of_war_factory cost row passed")
    artifact_transfer = PROVEN_CALL_TRANSFERS[
        ("philai.obj", 0x10E3F8, "buy_artifacts")]
    artifact_caller = """\
upgrade_creatures(current_hero, current_town);
buy_special_building(current_hero, current_town);
"""
    artifact_receiver = """\
if (!current_town->HasBuilding(SPECIAL_BUILDING_ID, 1)) {
    current_town->buy_building(SPECIAL_BUILDING_ID);
}
buy_artifacts(current_hero, gpGame->field_1f664, market_count);
"""
    if not transfer_satisfied(
            artifact_transfer, artifact_caller, artifact_receiver,
            {artifact_transfer.receiver_va}):
        failures.append("exact artifact-market call transfer did not pass")
    if transfer_satisfied(
            artifact_transfer, artifact_caller, artifact_receiver, set()):
        failures.append("non-exact artifact-market receiver passed")
    if transfer_satisfied(
            artifact_transfer, artifact_caller,
            artifact_receiver.replace(
                "buy_artifacts(current_hero, gpGame->field_1f664, "
                "market_count);", ""),
            {artifact_transfer.receiver_va}):
        failures.append("erased receiver buy_artifacts call passed")
    if transfer_satisfied(
            artifact_transfer,
            artifact_caller.replace(
                "buy_special_building(current_hero, current_town);", ""),
            artifact_receiver, {artifact_transfer.receiver_va}):
        failures.append("erased AI_enter_town forwarding call passed")
    enter_town_key = ("philai.obj", 0x10E3F8)
    enter_town_probe = """\
if (current_hero->HasArtifact(ARTIFACT_HOLY_GRAIL)
    && !current_town->HasBuilding(HOLY_GRAIL_ID, 0)
    && current_town->is_legal_building(HOLY_GRAIL_ID)) {
    current_hero->remove_artifact(ARTIFACT_HOLY_GRAIL);
    current_town->BuildBuilding(HOLY_GRAIL_ID, 1, 1);
    if (gpGame->mapHeader.victoryCondition.CheckForGrailBuildingWin())
        CheckEndGame(0);
}
if (player->resources[GOLD] >= 500
    && current_town->HasBuilding(MAGE_GUILD_ID, 1)
    && !current_hero->IsWieldingArtifact(ARTIFACT_SPELLBOOK)) {
    type_artifact artifact(ARTIFACT_SPELLBOOK, -1);
    current_hero->GiveArtifact(&artifact, 1, 1);
    player->resources[GOLD] -= 500;
}
upgrade_creatures(current_hero, current_town);
complete_only_building_work();
buy_special_building(current_hero, current_town);
if (gpGame->setup.difficulty) {
    if (garrison_hero) {
        AI_swap_artifacts(current_hero, garrison_hero);
        AI_swap_artifacts(garrison_hero, current_hero);
    }
    buy_siege_engine(current_hero, current_town, BLACKSMITH_ID,
                     artifact_from_int(blacksmith_artifact));
    if (current_town->type == TOWN_STRONGHOLD)
        buy_siege_engine(current_hero, current_town, EXTRA_1_ID,
                         ARTIFACT_BALLISTA);
}
gpAdvManager->DemobilizeCurrHero(0, 0);
if (garrison_hero)
    current_town->ApplySpecialBuildingEffect(garrison_hero);
if (current_town->visitingHeroId != -1)
    current_town->ApplySpecialBuildingEffect(
        gpGame->GetHero(current_town->visitingHeroId));
"""
    if contract_violations(enter_town_probe, enter_town_key):
        failures.append("aligned AI_enter_town source shape did not pass")
    flattened_grail = enter_town_probe.replace(
        "if (current_hero->HasArtifact(ARTIFACT_HOLY_GRAIL)\n"
        "    && !current_town->HasBuilding(HOLY_GRAIL_ID, 0)\n"
        "    && current_town->is_legal_building(HOLY_GRAIL_ID)) {",
        "if (!current_hero->HasArtifact(ARTIFACT_HOLY_GRAIL)) return;\n"
        "if (current_town->HasBuilding(HOLY_GRAIL_ID, 0)) return;\n"
        "if (!current_town->is_legal_building(HOLY_GRAIL_ID)) return;\n{")
    if not any("nested Grail" in rule.description for rule in
               contract_violations(flattened_grail, enter_town_key)):
        failures.append("flattened AI_enter_town Grail guards passed")
    hoisted_artifact = enter_town_probe.replace(
        "if (player->resources[GOLD] >= 500",
        "type_artifact artifact(ARTIFACT_SPELLBOOK, -1);\n"
        "if (player->resources[GOLD] >= 500").replace(
            "    type_artifact artifact(ARTIFACT_SPELLBOOK, -1);\n", "")
    if not any("sole artifact local" in rule.description for rule in
               contract_violations(hoisted_artifact, enter_town_key)):
        failures.append("hoisted AI_enter_town artifact local passed")
    reordered_enter_town = enter_town_probe.replace(
        "upgrade_creatures(current_hero, current_town);\n"
        "complete_only_building_work();\n"
        "buy_special_building(current_hero, current_town);",
        "buy_special_building(current_hero, current_town);\n"
        "complete_only_building_work();\n"
        "upgrade_creatures(current_hero, current_town);")
    if not any("upgrade_creatures then" in rule.description for rule in
               contract_violations(reordered_enter_town, enter_town_key)):
        failures.append("reordered AI_enter_town upgrade helpers passed")
    split_visiting_effect = enter_town_probe.replace(
        "if (current_town->visitingHeroId != -1)\n"
        "    current_town->ApplySpecialBuildingEffect(\n"
        "        gpGame->GetHero(current_town->visitingHeroId));",
        "if (current_town->visitingHeroId != -1) {\n"
        "    hero* visiting = gpGame->GetHero(current_town->visitingHeroId);\n"
        "    current_town->ApplySpecialBuildingEffect(visiting);\n}")
    if not any("final nested GetHero" in rule.description for rule in
               contract_violations(split_visiting_effect, enter_town_key)):
        failures.append("split AI_enter_town final helper group passed")
    load_armies_key = ("cmbtmgr.obj", 0x5E09C)
    load_armies_probe = """\
int side;
for (side = 0; side < 2; side++) {
    hero* combat_hero = heroes[side];
    armyGroup* group = armyGroups[side];
    const unsigned char grouped =
        combat_hero && (combat_hero->formation & 1) && sideIsAI[side];
    const int layout = group->GetNumArmies() - 1;
    for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; i++) {
        if (armyGroups[side]->armies[i] == CREATURE_NONE)
            continue;
        int hex;
        army& thisArmy = armies[side][placed];
        choose_hex(grouped, layout, &hex);
        thisArmy.Init(group->armies[i], group->numTroops[i], combat_hero,
                      side, placed, hex, i);
        thisArmy.LoadResources();
    }
}
"""
    if contract_violations(load_armies_probe, load_armies_key):
        failures.append("aligned LoadArmies local shape did not pass")
    loop_scoped_side = load_armies_probe.replace(
        "int side;\nfor (side = 0;", "for (int side = 0;")
    if not any("procedure-scope side" in rule.description for rule in
               contract_violations(loop_scoped_side, load_armies_key)):
        failures.append("loop-scoped LoadArmies side passed")
    mutable_layout = load_armies_probe.replace(
        "const unsigned char grouped", "unsigned char grouped").replace(
            "const int layout", "int layout")
    if not any("const grouped" in rule.description for rule in
               contract_violations(mutable_layout, load_armies_key)):
        failures.append("mutable LoadArmies grouped/layout locals passed")
    direct_army_access = load_armies_probe.replace(
        "int hex;\n        army& thisArmy = armies[side][placed];",
        "int hex;").replace("thisArmy.", "armies[side][placed].")
    if not any("army-reference thisArmy" in rule.description for rule in
               contract_violations(direct_army_access, load_armies_key)):
        failures.append("flattened LoadArmies thisArmy reference passed")
    next_army_key = ("cmbtmgr.obj", 0x5F518)
    next_army_probe = """\
if (stack->field_4f0 && stack->IsIncapacitated())
    continue;
"""
    if contract_violations(next_army_probe, next_army_key):
        failures.append("aligned NextArmy incapacity gate did not pass")
    unguarded_incapacity = next_army_probe.replace(
        "stack->field_4f0 && ", "")
    if not any("field_4f0 guard" in rule.description for rule in
               contract_violations(unguarded_incapacity, next_army_key)):
        failures.append("unguarded NextArmy incapacity helper passed")
    flattened_incapacity = next_army_probe.replace(
        "stack->IsIncapacitated()",
        "stack->disabled_290 || stack->disabled_2b0 || "
        "stack->disabled_2c0")
    if not any("helper boundary" in rule.description for rule in
               contract_violations(flattened_incapacity, next_army_key)):
        failures.append("flattened NextArmy incapacity helper passed")
    attacker_bonus_key = ("army.obj", 0x4868C)
    attacker_bonus_probe = """\
std::string text;
const char* creature_name;
creature_name = GetName();
text = format_string(gpGeneralText->GetText(366), creature_name);
"""
    if contract_violations(attacker_bonus_probe, attacker_bonus_key):
        failures.append("aligned attacker-bonus GetName boundary did not pass")
    flattened_attacker_name = attacker_bonus_probe.replace(
        "creature_name = GetName();",
        "creature_name = GetArmyName(creatureType, numTroops);")
    if not any("helper boundary" in rule.description for rule in
               contract_violations(flattened_attacker_name,
                                   attacker_bonus_key)):
        failures.append("flattened attacker-bonus GetName boundary passed")
    fenced_attacker_name = attacker_bonus_probe.replace(
        "creature_name = GetName();",
        "#pragma inline_depth(0)\n"
        "creature_name = GetName();\n"
        "#pragma inline_depth()")
    if not any("inline-depth fence" in rule.description for rule in
               contract_violations(fenced_attacker_name,
                                   attacker_bonus_key)):
        failures.append("de-inlined attacker-bonus GetName boundary passed")
    set_next_key = ("cmbtmgr.obj", 0x5F934)
    set_next_probe = """\
currentSide = stack->get_controlling_side();
switch (stack->creatureType) {
case army::ARMY_CREATURE_WRAITH:
    hero* drained = heroes[1 - stack->get_controlling_side()];
    if (!IsQuickCombat()) {
        SAMPLE2 sample = LoadPlaySample("ManaDrai.wav");
        std::string result;
        if (stack->numTroops == 1)
            result = format_string(one, stack->GetName(), drained->name);
        else {
            std::string many = format_string(
                plural, stack->GetName(), drained->name);
            result.assign(many, 0, std::string::npos);
        }
        if (combatWindow)
            combatWindow->combat_message(result.c_str(), 1, 0);
        SpellEffect(77, stack, 100, 0);
        WaitEndSample(sample, -1);
    }
    break;
}
lastMovedArmy = 0;
GetControl();
"""
    if contract_violations(set_next_probe, set_next_key):
        failures.append("aligned SetNextArmy source shape did not pass")
    local_controlling_side = set_next_probe.replace(
        "stack->get_controlling_side()", "ControllingSide(stack)")
    if not any("get_controlling_side" in rule.description for rule in
               contract_violations(local_controlling_side, set_next_key)):
        failures.append("file-local SetNextArmy controlling-side copy passed")
    direct_creature_name = set_next_probe.replace(
        "stack->GetName()", "CreatureName(stack->creatureType, "
        "stack->numTroops)")
    if not any("GetName" in rule.description for rule in
               contract_violations(direct_creature_name, set_next_key)):
        failures.append("de-inlined SetNextArmy GetName calls passed")
    renamed_result = set_next_probe.replace("result", "message")
    if not any("sole result local" in rule.description for rule in
               contract_violations(renamed_result, set_next_key)):
        failures.append("renamed SetNextArmy result local passed")
    unnamed_control = set_next_probe.replace(
        "GetControl();", "Unnamed4782d0();")
    if not any("named GetControl" in rule.description for rule in
               contract_violations(unnamed_control, set_next_key)):
        failures.append("ordinal SetNextArmy GetControl tail passed")
    combat_results_key = ("combatresultswindow.obj", 0x68364)
    combat_results_probe = """\
gpCombatResultsWindow = this;
long amount;
TCreatureType type;
int iDeadArmyTypes[2][20];
int iDeadArmyNumTroops[2][20];
const hero* const my_hero = attacker;
int iTtlDeadArmies[2];
char cText[100];
int firstX;
if (my_hero) { char cTemp[150]; sprintf(cTemp, format); }
if (attacker) { int numMons = 0; army& stack = armies[0];
if (stack.creatureType != -1 &&
    stack.creatureType != CREATURE_ARROW_TOWER) { numMons++; } }
if (defender) { int numMons = 0; army& stack = armies[1];
if (stack.creatureType != -1 &&
    stack.creatureType != CREATURE_ARROW_TOWER) { numMons++; } }
""" + "\n".join(
        "const char* text%d = (*gpGeneralText)[%d];" % (index, index)
        for index in range(18)) + "\n" + "\n".join(
        "Widgets.push_back(widget%d);" % index for index in range(21)) + """
Widgets.push_back(new bitmapBorder(
    384, 506, 66, 32, BACKGROUND_ID, "Box64x30.pcx", 0x800));
button* accept = 0;
accept = new button();
accept->set_hotkey(28);
accept->set_hotkey(1);
int iMaxToShow = min(iTtlDeadArmies[0], 7);
int x = firstX + 42 * iMaxToShow;
if (creature != -1 && lost > 0) { int row; }
"""
    if contract_violations(combat_results_probe, combat_results_key):
        failures.append("aligned combat-results source shape did not pass")
    flattened_text = combat_results_probe.replace(
        "(*gpGeneralText)[0]", "gpGeneralText->GetText(0)", 1)
    flattened_rules = contract_violations(
        flattened_text, combat_results_key)
    if not any("eighteen" in rule.description for rule in flattened_rules):
        failures.append("de-inlined combat-results operator[] count passed")
    if not any("direct GetText" in rule.description
               for rule in flattened_rules):
        failures.append("combat-results direct GetText substitution passed")
    collapsed_amount = combat_results_probe.replace("long amount;\n", "")
    if not any("function-scope amount" in rule.description for rule in
               contract_violations(collapsed_amount, combat_results_key)):
        failures.append("collapsed combat-results amount local passed")
    mutable_hero_pointer = combat_results_probe.replace(
        "const hero* const my_hero", "const hero* my_hero")
    if not any("const selected-hero pointer" in rule.description for rule in
               contract_violations(mutable_hero_pointer,
                                   combat_results_key)):
        failures.append("mutable combat-results my_hero pointer passed")
    short_temp = combat_results_probe.replace(
        "char cTemp[150];", "char cTemp[100];")
    if not any("cTemp[150]" in rule.description for rule in
               contract_violations(short_temp, combat_results_key)):
        failures.append("short combat-results cTemp passed")
    renamed_combat_locals = combat_results_probe.replace(
        "numMons", "liveStacks").replace(
            "iMaxToShow", "shown").replace("int x =", "int rowX =")
    renamed_rules = contract_violations(
        renamed_combat_locals, combat_results_key)
    for identity in ("numMons", "iMaxToShow", "inner x"):
        if not any(identity in rule.description for rule in renamed_rules):
            failures.append("renamed combat-results %s local passed" %
                            identity)
    split_loss_gate = combat_results_probe.replace(
        "if (creature != -1 && lost > 0) { int row; }",
        "if (creature == -1) continue;\nif (lost <= 0) continue;\n"
        "int row;")
    if not any("positive loss-aggregation scope" in rule.description
               for rule in contract_violations(
                   split_loss_gate, combat_results_key)):
        failures.append("split combat-results loss gates passed")
    split_strongest_gate = combat_results_probe.replace(
        "if (stack.creatureType != -1 &&\n"
        "    stack.creatureType != CREATURE_ARROW_TOWER) { numMons++; }",
        "if (stack.creatureType == -1) continue;\n"
        "if (stack.creatureType == CREATURE_ARROW_TOWER) continue;\n"
        "numMons++;")
    if not any("positive strongest-stack scopes" in rule.description
               for rule in contract_violations(
                   split_strongest_gate, combat_results_key)):
        failures.append("split combat-results strongest-stack gates passed")
    split_accept_box = combat_results_probe.replace(
        "Widgets.push_back(new bitmapBorder(\n"
        "    384, 506, 66, 32, BACKGROUND_ID, \"Box64x30.pcx\", 0x800));",
        "bitmapBorder* acceptBox = new bitmapBorder(\n"
        "    384, 506, 66, 32, BACKGROUND_ID, \"Box64x30.pcx\", 0x800);\n"
        "Widgets.push_back(acceptBox);")
    if not any("line 319's accept-box" in rule.description
               for rule in contract_violations(
                   split_accept_box, combat_results_key)):
        failures.append("split combat-results accept-box group passed")
    collapsed_accept = combat_results_probe.replace(
        "button* accept = 0;\naccept = new button();",
        "button* accept = new button();")
    if not any("source-gap line 320" in rule.description
               for rule in contract_violations(
                   collapsed_accept, combat_results_key)):
        failures.append("collapsed combat-results accept initializer passed")
    upgrade_key = ("philai.obj", 0x10D684)
    upgrade_probe = """\
int difference[NUM_RESOURCES];
long amount;
long dwelling;
const int* upgrade_cost;
const int* base_cost;
for (dwelling = 0; dwelling < TOWN_DWELLING_COUNT; ++dwelling) {
    if (!current_town->HasBuilding(DWELLING_0_UPG_ID + dwelling, 1))
        continue;
    base_cost = gCreatureRecords + base_type * CREATURE_RECORD_DWORDS;
    upgrade_cost = gCreatureRecords + upgrade * CREATURE_RECORD_DWORDS;
    amount = current_hero->army.numTroops[slot];
}
"""
    if contract_violations(upgrade_probe, upgrade_key):
        failures.append("aligned upgrade_creatures local contract did not pass")
    collapsed_upgrade = upgrade_probe.replace(
        "int difference[NUM_RESOURCES];\nlong amount;\nlong dwelling;\n"
        "const int* upgrade_cost;\nconst int* base_cost;\n"
        "for (dwelling = 0;", "for (long dwelling = 0;")
    if not any("function-scope" in rule.description for rule in
               contract_violations(collapsed_upgrade, upgrade_key)):
        failures.append("collapsed upgrade_creatures locals passed")
    reordered_upgrade = upgrade_probe.replace(
        "base_cost = gCreatureRecords + base_type * "
        "CREATURE_RECORD_DWORDS;\n    upgrade_cost = gCreatureRecords + "
        "upgrade * CREATURE_RECORD_DWORDS;",
        "upgrade_cost = gCreatureRecords + upgrade * "
        "CREATURE_RECORD_DWORDS;\n    base_cost = gCreatureRecords + "
        "base_type * CREATURE_RECORD_DWORDS;")
    if not any("base-cost" in rule.description for rule in
               contract_violations(reordered_upgrade, upgrade_key)):
        failures.append("reordered upgrade_creatures costs passed")
    bridged_upgrade = upgrade_probe.replace(
        "HasBuilding(DWELLING_0_UPG_ID + dwelling, 1)",
        "HasBuilding(building_id_from_int(DWELLING_0_UPG_ID + dwelling), 1)")
    if not any("non-attested enum bridge" in rule.description for rule in
               contract_violations(bridged_upgrade, upgrade_key)):
        failures.append("non-attested upgrade_creatures enum bridge passed")
    experience_key = ("philai.obj", 0x10FEB8)
    experience_probe = """\
int increment = current_hero->GetExperienceIncrement();
float army_value = float(current_army.get_AI_value());
return (float(gHeroGoldCost) + army_value) / float(increment * 40);
"""
    if contract_violations(experience_probe, experience_key):
        failures.append("aligned value_of_experience contract did not pass")
    flattened_experience = experience_probe.replace(
        "float army_value = float(current_army.get_AI_value());\n", "").replace(
            "army_value", "float(current_army.get_AI_value())")
    if not any("separate statement" in rule.description for rule in
               contract_violations(flattened_experience, experience_key)):
        failures.append("flattened value_of_experience conversion passed")
    pointer_experience = experience_probe.replace(
        "current_army.get_AI_value()", "current_army->get_AI_value()")
    if not any("const-reference" in rule.description for rule in
               contract_violations(pointer_experience, experience_key)):
        failures.append("pointer-shaped value_of_experience army passed")
    static_increment = experience_probe.replace(
        "current_hero->GetExperienceIncrement()",
        "hero::GetExperienceIncrement(current_hero->level)")
    if not any("no-argument const hero accessor" in rule.description
               for rule in contract_violations(static_increment,
                                                experience_key)):
        failures.append("static value_of_experience increment call passed")
    mine_pool_key = ("game.obj", 0xA3E5C)
    if contract_violations("int count;\nint x;\n", mine_pool_key):
        failures.append("aligned LoadMinePool signed x local did not pass")
    if not contract_violations(
            "int count;\nunsigned int x;\n", mine_pool_key):
        failures.append("unsigned LoadMinePool x escaped source-shape gate")
    load_victory_key = ("game.obj", 0xAEB64)
    load_victory_probe = """\
int int_buffer;
int count;
char char_buffer;
count = infile->Read(&char_buffer, sizeof(char_buffer));
victoryCondition.AllowNormalVictory = char_buffer != 0;
count = infile->Read(&char_buffer, sizeof(char_buffer));
victoryCondition.AppliesToComputer = char_buffer != 0;
switch (type) {}
"""
    if contract_violations(load_victory_probe, load_victory_key):
        failures.append("aligned loadVictoryCondition source shape did not pass")
    reordered_load_victory = load_victory_probe.replace(
        "int int_buffer;\nint count;\nchar char_buffer;",
        "int count;\nchar char_buffer;\nint int_buffer;")
    if not any("raw NB11 order" in rule.description for rule in
               contract_violations(reordered_load_victory,
                                   load_victory_key)):
        failures.append("reordered loadVictoryCondition locals passed")
    flattened_load_victory = load_victory_probe.replace(
        "count = infile->Read", "infile->Read")
    if not any("two leading count/read assignments" in rule.description
               for rule in contract_violations(flattened_load_victory,
                                                load_victory_key)):
        failures.append("flattened loadVictoryCondition count reads passed")
    claim_town_key = ("game.obj", 0xB1230)
    claim_town_probe = """\
town* thisTown = &towns[townId];
long old_owner = thisTown->owner;
long i;
if (old_owner == newPlayerOwner) return;
for (i = 0; i < generators.size(); i++) { remove_bonus(); }
int team = claim_town_team(this, thisTown->owner);
if (team >= 0 && IsComputerTeam(team)) { update_team(); }
for (i = 0; i < generators.size(); i++) { update_bonus(); }
"""
    if contract_violations(claim_town_probe, claim_town_key):
        failures.append("aligned ClaimTown source shape did not pass")
    renamed_claim_town = claim_town_probe.replace(
        "thisTown", "whichTown").replace("old_owner", "oldOwner")
    if not any("thisTown, old_owner" in rule.description for rule in
               contract_violations(renamed_claim_town, claim_town_key)):
        failures.append("renamed ClaimTown NB11 locals passed")
    flattened_computer_team = claim_town_probe.replace(
        "IsComputerTeam(team)", "!is_human_ally(team)")
    if not any("IsComputerTeam source boundary" in rule.description
               for rule in contract_violations(
                   flattened_computer_team, claim_town_key)):
        failures.append("flattened ClaimTown IsComputerTeam boundary passed")
    split_claim_town_i = claim_town_probe.replace(
        "for (i = 0;", "for (long i = 0;")
    if not any("sole i local" in rule.description for rule in
               contract_violations(split_claim_town_i, claim_town_key)):
        failures.append("split ClaimTown generator-loop i locals passed")
    new_hero_key = ("game.obj", 0xA6CD4)
    new_hero_probe = """\
int hero_class;
long total_count;
long choice = 0;
long counts[18];
int hero_id;
long weights[18];
long aligned_count;
total_count = 0;
if (excluded < kNumHeroClasses && counts[excluded] < total_count) {
    weights[excluded] = 0;
}
if (prefer_alignment) {
    aligned_count = 0;
    count_aligned();
}
choice = Random(1, totalWeight);
choice -= weights[hero_class];
choice = Random(1, counts[hero_class]);
if (--choice == 0) return hero_id;
"""
    if contract_violations(new_hero_probe, new_hero_key):
        failures.append("aligned GetNewHeroId source shape did not pass")
    camel_new_hero = new_hero_probe.replace(
        "hero_class", "heroClass").replace(
            "total_count", "totalCount").replace(
                "hero_id", "heroId").replace(
                    "aligned_count", "alignedCount")
    if not any("raw NB11 order" in rule.description for rule in
               contract_violations(camel_new_hero, new_hero_key)):
        failures.append("renamed GetNewHeroId NB11 locals passed")
    scoped_new_hero = new_hero_probe.replace(
        "long aligned_count;\n", "").replace(
            "    aligned_count = 0;", "    long aligned_count = 0;")
    if not any("function lifetime" in rule.description for rule in
               contract_violations(scoped_new_hero, new_hero_key)):
        failures.append("scoped GetNewHeroId aligned_count local passed")
    split_new_hero_choice = new_hero_probe.replace(
        "choice = Random(1, counts[hero_class]);",
        "long hero_choice = Random(1, counts[hero_class]);").replace(
            "if (--choice == 0)", "if (--hero_choice == 0)")
    if not any("shared choice local" in rule.description for rule in
               contract_violations(split_new_hero_choice, new_hero_key)):
        failures.append("split GetNewHeroId choice phases passed")
    prison_key = ("events.obj", 0x94760)
    prison_probe = """\
int heroID = cell->extraInfo;
unsigned char OldColorCycling = gUnnamed67f574;
gUnnamed67f574 = 0;
unsigned char OldAnimCtrPaused = animCtrPaused;
animCtrPaused = 1;
gpGame->record_show_hero(prisoner, current_hero->owner, point, 0);
prisoner->owner = current_hero->owner;
gpGame->heroAvailability[heroID] = current_hero->owner;
gpGame->heroPoolMap[heroID][current_hero->owner] = 1;
gpCurrentPlayer->heroes[gpCurrentPlayer->numHeroes] = heroID;
++gpCurrentPlayer->numHeroes;
prisoner->x = point.x;
prisoner->y = point.y;
prisoner->z = point.z;
prisoner->flags = 0;
prisoner->facing = hero::kFacingE;
prisoner->movePoints = prisoner->GetMobility();
prisoner->maxMovePoints = prisoner->movePoints;
cell->is_trigger = 0;
cell->type_value = 0;
prisoner->obscure_cell();
FizzleCenter(FIZZLE_SOUND_PICKUP);
animCtrPaused = OldAnimCtrPaused;
gUnnamed67f574 = OldColorCycling;
"""
    if contract_violations(prison_probe, prison_key):
        failures.append("aligned DoEventPrison source shape did not pass")
    flattened_prison_id = prison_probe.replace(
        "int heroID = cell->extraInfo;\n", "")
    if not any("THeroID source local" in rule.description for rule in
               contract_violations(flattened_prison_id, prison_key)):
        failures.append("flattened DoEventPrison heroID local passed")
    swapped_prison_saves = prison_probe.replace(
        "unsigned char OldColorCycling = gUnnamed67f574;\n"
        "gUnnamed67f574 = 0;\n"
        "unsigned char OldAnimCtrPaused = animCtrPaused;\n"
        "animCtrPaused = 1;",
        "unsigned char OldAnimCtrPaused = animCtrPaused;\n"
        "animCtrPaused = 1;\n"
        "unsigned char OldColorCycling = gUnnamed67f574;\n"
        "gUnnamed67f574 = 0;")
    if not any("save/disable and restore order" in rule.description
               for rule in contract_violations(swapped_prison_saves,
                                                prison_key)):
        failures.append("reordered DoEventPrison save locals passed")
    swapped_prison_coords = prison_probe.replace(
        "prisoner->x = point.x;\nprisoner->y = point.y;",
        "prisoner->y = point.y;\nprisoner->x = point.x;")
    if not any("rescued-hero statement order" in rule.description
               for rule in contract_violations(swapped_prison_coords,
                                                prison_key)):
        failures.append("reordered DoEventPrison coordinates passed")
    single_enchantment_key = ("ai_tactical.obj", 0x40BB8)
    single_enchantment_probe = """\
TEnchantValue value_func = get_enchantment_function(choice->spell);
long value = (this->*value_func)(target, *choice);
"""
    if contract_violations(single_enchantment_probe,
                           single_enchantment_key):
        failures.append(
            "aligned consider_single_enchantment value_func did not pass")
    renamed_value_func = single_enchantment_probe.replace(
        "value_func", "value_of")
    if not contract_violations(renamed_value_func, single_enchantment_key):
        failures.append(
            "renamed consider_single_enchantment value_func passed")
    hiring_value_key = ("ai_player.obj", 0x34FB8)
    hiring_value_probe = """\
short player_id = current_town->owner;
playerData* player = &gpGame->players[current_town->owner];
armyGroup hero_army = candidate->army;
armyGroup town_army = current_town->get_army();
type_AI_creature_purchaser purchaser(player_id, current_town);
int resources[7];
memcpy(resources, player->resources, sizeof(resources));
short population[14];
memcpy(population, current_town->population, sizeof(population));
std::vector<HeroDestination> destinations;
std::vector<pathCell*> monsters;
long total_value = 0;
HeroDestination destination;
pathCell* monster_cell;
unsigned int i;
for (i = 0; i < destinations.size(); ++i) {}
long heroes_touched = 1;
long best_hero_value = 0;
for (int hero_index = 0; hero_index < player->numHeroes; ++hero_index) {}
candidate->owner = -1;
candidate->army = hero_army;
memcpy(player->resources, resources, sizeof(resources));
memcpy(current_town->population, population, sizeof(population));
"""
    if contract_violations(hiring_value_probe, hiring_value_key):
        failures.append("aligned value_of_hiring source shape did not pass")
    hiring_value_mutations = (
        hiring_value_probe.replace(
            "armyGroup town_army = current_town->get_army();",
            "armyGroup town_army = "
            "static_cast<const town*>(current_town)->get_army();"),
        hiring_value_probe.replace("short player_id", "int player_id", 1),
        hiring_value_probe.replace("pathCell* monster_cell;", "pathCell* cell;"),
        hiring_value_probe.replace(
            "long heroes_touched = 1;\nlong best_hero_value = 0;",
            "long best_hero_value = 0;\nlong heroes_touched = 1;"),
        hiring_value_probe.replace("int resources[7];", "int resources[8];"),
    )
    if any(not contract_violations(probe, hiring_value_key)
           for probe in hiring_value_mutations):
        failures.append("broken value_of_hiring source shape passed")
    prohibited_key = ("ai_player.obj", 0x2F694)
    prohibited_probe = """\
long human_strength;
int income[7];
short i;
int resources[7];
current_town->get_growth_rate(i);
GetMonsterCost(i, resources);
human_strength = 0;
sum_player_dwellings(0);
"""
    if contract_violations(prohibited_probe, prohibited_key):
        failures.append("aligned fill_prohibited_array locals did not pass")
    reordered_prohibited = prohibited_probe.replace(
        "long human_strength;\nint income[7];\nshort i;\nint resources[7];",
        "int income[7];\nshort i;\nint resources[7];\nlong human_strength;")
    if not any("CodeView declaration order" in rule.description for rule in
               contract_violations(reordered_prohibited, prohibited_key)):
        failures.append("reordered fill_prohibited_array locals passed")
    early_prohibited_zero = prohibited_probe.replace(
        "long human_strength;", "long human_strength = 0;").replace(
            "\nhuman_strength = 0;\n", "\n")
    if not any("after the initial dwelling-growth cost pass" in
               rule.description for rule in contract_violations(
                   early_prohibited_zero, prohibited_key)):
        failures.append("early fill_prohibited_array human_strength passed")
    add_garrison_key = ("game.obj", 0xA4EE8)
    add_garrison_probe = """\
int i;
hero* our_hero;
int found;
if (our_town->visitingHeroId < 0)
    return 0;
our_hero = gpGame->GetHero(our_town->visitingHeroId);
if (!our_hero->army.Merge(const_cast<armyGroup*>(
        &static_cast<const town*>(our_town)->get_army())))
    return 0;
gpGame->record_hide_hero(our_hero, our_hero->owner, 0);
if (bVideoPaused) {
    CMCHideHero hideHero(our_hero->id);
    SendMapChange(&hideHero);
}
found = FindHero(our_hero->id);
our_hero->restore_cell();
for (i = found; i < numHeroes - 1; ++i)
    heroes[i] = heroes[i + 1];
if (currHeroId == our_hero->id) {
    currHeroId = -1;
    if (gNetLocalGamePos == our_hero->owner) {
        gpAdvManager->drawCursor = 0;
    }
}
--numHeroes;
our_town->garrisonHeroId = our_hero->id;
our_town->visitingHeroId = -1;
return 1;
"""
    if contract_violations(add_garrison_probe, add_garrison_key):
        failures.append("aligned add_garrison_hero shape did not pass")
    broken_add_garrison_probes = (
        (add_garrison_probe.replace(
            "int i;\nhero* our_hero;\nint found;",
            "hero* our_hero;\nint found;\nint i;"),
         "procedure-scope i, our_hero and found locals"),
        (add_garrison_probe.replace("record_hide_hero", "GameFn_0049C720"),
         "GetHero, Merge, record_hide_hero"),
        (add_garrison_probe.replace("for (i = found;", "for (int i = found;"),
         "shared i roster-shift loop"),
        (add_garrison_probe.replace(
            "if (currHeroId == our_hero->id) {",
            "--numHeroes;\nif (currHeroId == our_hero->id) {").replace(
                "}\n--numHeroes;\nour_town->garrisonHeroId",
                "}\nour_town->garrisonHeroId"),
         "current-hero clear before the roster decrement"),
    )
    for probe, description in broken_add_garrison_probes:
        if not any(description in rule.description for rule in
                   contract_violations(probe, add_garrison_key)):
            failures.append("broken add_garrison_hero " + description
                            + " source shape passed")
    player_save_key = ("game.obj", 0xA55A8)
    player_save_probe = """\
unsigned long flags;
int number;
int count;
int x;
unsigned char flag;
char value;
""" + "count = outfile->Write(&value, sizeof(value));\n" * 20 + """\
for (x = 0; x < 8; x++) {}
for (x = 0; x < 0x48; x++) {}
for (x = 0; x < 7; x++) {}
"""
    if contract_violations(player_save_probe, player_save_key):
        failures.append("aligned playerData::save count/x shape did not pass")
    reordered_player_save = player_save_probe.replace(
        "int count;\nint x;", "int x;\nint count;")
    if not any("declaration order" in rule.description for rule in
               contract_violations(reordered_player_save, player_save_key)):
        failures.append("reordered playerData::save locals passed")
    flattened_player_save = player_save_probe.replace(
        "count = outfile->Write(&value, sizeof(value));\n", "", 1)
    if not any("twenty Dreamcast write results" in rule.description
               for rule in contract_violations(flattened_player_save,
                                                player_save_key)):
        failures.append("flattened playerData::save Write result passed")
    reused_count_loop = player_save_probe.replace(
        "for (x =", "for (count =")
    if not any("all three" in rule.description for rule in
               contract_violations(reused_count_loop, player_save_key)):
        failures.append("playerData::save count-as-loop-index passed")
    setup_puzzle_key = ("game.obj", 0xA6350)
    setup_puzzle_probe = """\
long piece;
float fPercentObelisksFound;
float fPercentExtraPieces;
int i;
long j;
int iExtraPieces;
int iPiecesRemoved;
"""
    if contract_violations(setup_puzzle_probe, setup_puzzle_key):
        failures.append(
            "aligned SetupPuzzlePieces local declaration order did not pass")
    reordered_setup_puzzle = setup_puzzle_probe.replace(
        "long piece;\nfloat fPercentObelisksFound;\n"
        "float fPercentExtraPieces;\nint i;\nlong j;\n"
        "int iExtraPieces;\nint iPiecesRemoved;",
        "int i;\nlong j;\nlong piece;\nint iExtraPieces;\n"
        "int iPiecesRemoved;\nfloat fPercentObelisksFound;\n"
        "float fPercentExtraPieces;")
    if not contract_violations(reordered_setup_puzzle, setup_puzzle_key):
        failures.append("reordered SetupPuzzlePieces locals passed")
    per_week_key = ("game.obj", 0xB41E0)
    per_week_probe = """\
hero* obscuring_hero;
int iAlign;
TCreatureType alternate_bonus;
long bonus_amount;
int x;
int y;
int i;
int z;
TCreatureType bonus_creature;
NewmapCell* map_cell;
case MONSTER: {
    if (!(map_cell->extraInfo & 0x40000)) {
        int iCount = map_cell->extraInfo & 0xfff;
        int iIncrease = iCount / 10;
    }
}
case FOUNTAIN_OF_FORTUNE: {
    int luck_bonus = Random(0, 3);
}
for (i = 0; i < HERO_COUNT; ++i) {
    hero* currHero = &heroes[i];
}
if (towns[i].IsCastle()) {}
current_town->HasBuilding(EXTRA_1_ID, 0);
"""
    if contract_violations(per_week_probe, per_week_key):
        failures.append("aligned PerWeek source shape did not pass")
    reordered_per_week = per_week_probe.replace(
        "hero* obscuring_hero;\nint iAlign;\n"
        "TCreatureType alternate_bonus;\nlong bonus_amount;",
        "TCreatureType alternate_bonus;\nlong bonus_amount;\n"
        "int iAlign;\nhero* obscuring_hero;")
    if not any("procedure-scope locals" in rule.description for rule in
               contract_violations(reordered_per_week, per_week_key)):
        failures.append("reordered PerWeek procedure locals passed")
    hoisted_per_week_count = per_week_probe.replace(
        "case MONSTER: {\n    if (!(map_cell->extraInfo & 0x40000)) {\n"
        "        int iCount = map_cell->extraInfo & 0xfff;\n"
        "        int iIncrease = iCount / 10;",
        "int iCount;\nint iIncrease;\ncase MONSTER: {\n"
        "    if (!(map_cell->extraInfo & 0x40000)) {")
    if not any("MONSTER growth scope" in rule.description for rule in
               contract_violations(hoisted_per_week_count, per_week_key)):
        failures.append("hoisted PerWeek monster locals passed")
    hoisted_per_week_luck = per_week_probe.replace(
        "case FOUNTAIN_OF_FORTUNE: {\n"
        "    int luck_bonus = Random(0, 3);",
        "int luck_bonus;\ncase FOUNTAIN_OF_FORTUNE: {\n"
        "    luck_bonus = Random(0, 3);")
    if not any("FOUNTAIN_OF_FORTUNE scope" in rule.description for rule in
               contract_violations(hoisted_per_week_luck, per_week_key)):
        failures.append("hoisted PerWeek luck local passed")
    flattened_per_week_castle = per_week_probe.replace(
        "towns[i].IsCastle()",
        "towns[i].built & bitNumber[CASTLE_FORT_ID]")
    if not any("IsCastle helper boundary" in rule.description for rule in
               contract_violations(flattened_per_week_castle,
                                   per_week_key)):
        failures.append("flattened PerWeek IsCastle call passed")
    active_summoning_portal = per_week_probe.replace(
        "HasBuilding(EXTRA_1_ID, 0)", "HasBuilding(EXTRA_1_ID, 1)")
    if not any("built-mask Summoning Portal" in rule.description for rule in
               contract_violations(active_summoning_portal, per_week_key)):
        failures.append("active-mask PerWeek Summoning Portal test passed")
    is_castle_key = ("game.obj", 0xBCC40)
    is_castle_probe = """\
return HasBuilding(CASTLE_FORT_ID, 0)
    || HasBuilding(CASTLE_CITADEL_ID, 0)
    || HasBuilding(CASTLE_CASTLE_ID, 0);
"""
    if contract_violations(is_castle_probe, is_castle_key):
        failures.append("aligned town::IsCastle body did not pass")
    flattened_is_castle = is_castle_probe.replace(
        "HasBuilding(CASTLE_FORT_ID, 0)",
        "(built & bitNumber[CASTLE_FORT_ID]) != 0")
    if not contract_violations(flattened_is_castle, is_castle_key):
        failures.append("flattened town::IsCastle HasBuilding call passed")
    can_cast_spell_key = ("army.obj", 0x4BEEC)
    can_cast_spell_probe = """\
case CREATURE_MASTER_GENIE:
    return target && get_valid_caliph_spells(target) > 0;
"""
    if contract_violations(can_cast_spell_probe, can_cast_spell_key):
        failures.append("aligned can_cast_spell Genie arm did not pass")
    flattened_can_cast_spell = can_cast_spell_probe.replace(
        "get_valid_caliph_spells(target) > 0",
        "count_valid_caliph_spells_inline(target) > 0")
    if not contract_violations(flattened_can_cast_spell,
                               can_cast_spell_key):
        failures.append("flattened can_cast_spell helper boundary passed")
    valid_caliph_key = ("army.obj", 0x4C374)
    valid_caliph_probe = """\
long count = 0;
for (SpellID spell = 10; spell < 70; spell++) {
    if (is_valid_caliph_spell(spell, target))
        count++;
}
return count;
"""
    if contract_violations(valid_caliph_probe, valid_caliph_key):
        failures.append("aligned get_valid_caliph_spells body did not pass")
    shortened_valid_caliph = valid_caliph_probe.replace(
        "spell < 70", "spell < 69")
    if not contract_violations(shortened_valid_caliph, valid_caliph_key):
        failures.append("shortened get_valid_caliph_spells roster passed")
    cast_caliph_key = ("army.obj", 0x4C3AC)
    cast_caliph_probe = """\
SpellID spell;
long count = get_valid_caliph_spells(target);
if (count == 0)
    return;
long pick = Random(1, count);
for (spell = 10; spell < 70; spell++) {
}
"""
    if contract_violations(cast_caliph_probe, cast_caliph_key):
        failures.append("aligned cast_caliph_spell helper shape did not pass")
    reordered_cast_caliph = cast_caliph_probe.replace(
        "long count = get_valid_caliph_spells(target);\n"
        "if (count == 0)\n    return;",
        "long count = 0;\nif (count == 0)\n    return;\n"
        "count = get_valid_caliph_spells(target);")
    if not contract_violations(reordered_cast_caliph, cast_caliph_key):
        failures.append("reordered cast_caliph_spell helper call passed")
    read_map_objects_key = ("mapcell.obj", 0xF2C20)
    read_map_objects_probe = """\
for (int i = 0; i < objectTypes.size(); ++i) {
    int status = readObjectType(infile, &objectTypes[i]);
}
NewfullMapFn_005042C0();
IncProgressBar(1);
"""
    if contract_violations(read_map_objects_probe, read_map_objects_key):
        failures.append(
            "aligned readMapObjects Complete rebuild boundary did not pass")
    flattened_read_rebuild = read_map_objects_probe.replace(
        "NewfullMapFn_005042C0();\n", "")
    if not any("explicit helper call" in rule.description for rule in
               contract_violations(flattened_read_rebuild,
                                   read_map_objects_key)):
        failures.append(
            "flattened readMapObjects Complete rebuild boundary passed")
    early_read_rebuild = read_map_objects_probe.replace(
        "for (int i", "NewfullMapFn_005042C0();\nfor (int i").replace(
            "\nNewfullMapFn_005042C0();\nIncProgressBar", "\nIncProgressBar")
    if not any("after the readObjectType loop" in rule.description for rule in
               contract_violations(early_read_rebuild,
                                   read_map_objects_key)):
        failures.append("early readMapObjects Complete rebuild passed")
    load_map_objects_key = ("mapcell.obj", 0xF318C)
    load_map_objects_probe = """\
for (int i = 0; i < objectTypes.size(); ++i) {
    if (loadObjectType(infile, &objectTypes[i]) < 0)
        return -1;
}
IncProgressBar(1);
NewfullMapFn_005042C0();
"""
    if contract_violations(load_map_objects_probe, load_map_objects_key):
        failures.append(
            "aligned loadMapObjects Complete rebuild boundary did not pass")
    flattened_load_rebuild = load_map_objects_probe.replace(
        "NewfullMapFn_005042C0();\n", "")
    if not any("explicit helper call" in rule.description for rule in
               contract_violations(flattened_load_rebuild,
                                   load_map_objects_key)):
        failures.append(
            "flattened loadMapObjects Complete rebuild boundary passed")
    early_load_rebuild = load_map_objects_probe.replace(
        "IncProgressBar(1);\nNewfullMapFn_005042C0();",
        "NewfullMapFn_005042C0();\nIncProgressBar(1);")
    if not any("following progress tick" in rule.description for rule in
               contract_violations(early_load_rebuild,
                                   load_map_objects_key)):
        failures.append("early loadMapObjects Complete rebuild passed")
    load_boats_key = ("game.obj", 0xA46E8)
    load_boats_probe = """\
unsigned short ushort_buffer;
int count;
int x;
unsigned char uchar_buffer;
char char_buffer;
count = infile->Read(&uchar_buffer, sizeof(uchar_buffer));
count = infile->Read(&char_buffer, sizeof(char_buffer));
boats[x].allocated = char_buffer != 0;
count = infile->Read(&uchar_buffer, sizeof(uchar_buffer));
boats[x].id = uchar_buffer;
count = infile->Read(&char_buffer, sizeof(char_buffer));
boats[x].type = char_buffer;
count = infile->Read(&char_buffer, sizeof(char_buffer));
boats[x].facing = char_buffer;
count = infile->Read(&char_buffer, sizeof(char_buffer));
boats[x].playerOwner = char_buffer;
count = infile->Read(&ushort_buffer, sizeof(ushort_buffer));
boats[x].occupying_hero = ushort_buffer;
count = infile->Read(&char_buffer, sizeof(char_buffer));
boats[x].occupied = char_buffer != 0;
"""
    if contract_violations(load_boats_probe, load_boats_key):
        failures.append("aligned LoadBoatPool serialization shape did not pass")
    reordered_load_boats = load_boats_probe.replace(
        "unsigned short ushort_buffer;\nint count;\nint x;\n"
        "unsigned char uchar_buffer;\nchar char_buffer;",
        "int count;\nint x;\nunsigned char uchar_buffer;\n"
        "char char_buffer;\nunsigned short ushort_buffer;")
    if not any("CodeView record order" in rule.description for rule in
               contract_violations(reordered_load_boats, load_boats_key)):
        failures.append("reordered LoadBoatPool locals passed")
    direct_load_read = load_boats_probe.replace(
        "count = infile->Read", "infile->Read", 1)
    if not any("every Dreamcast read result" in rule.description for rule in
               contract_violations(direct_load_read, load_boats_key)):
        failures.append("discarded LoadBoatPool read result passed")
    wrong_load_field = load_boats_probe.replace(
        "boats[x].id = uchar_buffer;", "boats[x].id = char_buffer;")
    if not any("typed boat-field assignment" in rule.description for rule in
               contract_violations(wrong_load_field, load_boats_key)):
        failures.append("wrong LoadBoatPool id buffer passed")
    save_boats_key = ("game.obj", 0xA4980)
    save_boats_probe = """\
unsigned short ushort_buffer;
int count;
int x;
unsigned char uchar_buffer;
char char_buffer;
count = outfile->Write(&uchar_buffer, sizeof(uchar_buffer));
count = outfile->Write(&char_buffer, sizeof(char_buffer));
count = outfile->Write(&uchar_buffer, sizeof(uchar_buffer));
count = outfile->Write(&char_buffer, sizeof(char_buffer));
count = outfile->Write(&char_buffer, sizeof(char_buffer));
count = outfile->Write(&char_buffer, sizeof(char_buffer));
count = outfile->Write(&ushort_buffer, sizeof(ushort_buffer));
count = outfile->Write(&char_buffer, sizeof(char_buffer));
"""
    if contract_violations(save_boats_probe, save_boats_key):
        failures.append("aligned SaveBoatPool serialization shape did not pass")
    reordered_save_boats = save_boats_probe.replace(
        "unsigned short ushort_buffer;\nint count;\nint x;\n"
        "unsigned char uchar_buffer;\nchar char_buffer;",
        "int count;\nint x;\nunsigned char uchar_buffer;\n"
        "char char_buffer;\nunsigned short ushort_buffer;")
    if not any("CodeView record order" in rule.description for rule in
               contract_violations(reordered_save_boats, save_boats_key)):
        failures.append("reordered SaveBoatPool locals passed")
    direct_save_write = save_boats_probe.replace(
        "count = outfile->Write", "outfile->Write", 1)
    if not any("every Dreamcast write result" in rule.description for rule in
               contract_violations(direct_save_write, save_boats_key)):
        failures.append("discarded SaveBoatPool write result passed")
    randomize_university_key = ("game.obj", 0xAC048)
    randomize_university_probe = """\
int university[4];
std::bitset<28> availableSkills;
long choice;
int availableCount;
long i;
TSecondarySkill skill;
"""
    if contract_violations(randomize_university_probe,
                           randomize_university_key):
        failures.append(
            "aligned randomize_university shared-local shape did not pass")
    reordered_randomize_university = randomize_university_probe.replace(
        "long i;\nTSecondarySkill skill;",
        "TSecondarySkill skill;\nlong i;")
    if not contract_violations(reordered_randomize_university,
                               randomize_university_key):
        failures.append(
            "reordered randomize_university shared locals passed")
    int_randomize_skill = randomize_university_probe.replace(
        "TSecondarySkill skill;", "int skill;")
    if not contract_violations(int_randomize_skill,
                               randomize_university_key):
        failures.append("int randomize_university skill escaped shape gate")
    renamed_randomize_university = randomize_university_probe.replace(
        "int university[4];", "int selectedSkills[4];")
    if not contract_violations(renamed_randomize_university,
                               randomize_university_key):
        failures.append(
            "renamed randomize_university aggregate escaped shape gate")
    resized_randomize_university = randomize_university_probe.replace(
        "int university[4];", "int university[5];")
    if not contract_violations(resized_randomize_university,
                               randomize_university_key):
        failures.append(
            "resized randomize_university aggregate escaped shape gate")
    initialize_hordes_key = ("town.obj", 0x1664B0)
    initialize_hordes_probe = """\
effect->dwelling = slot;
slot += TOWN_DWELLING_COUNT;
effect[1].creature = gTownDwellingCreatures[creatureBase + slot];
effect[1].dwelling = slot;
const short* bonus = &effect->bonus;
effect[1].bonus = *bonus;
"""
    if contract_violations(initialize_hordes_probe,
                           initialize_hordes_key):
        failures.append(
            "aligned initialize_hordes statement order did not pass")
    reordered_initialize_hordes = initialize_hordes_probe.replace(
        "effect[1].dwelling = slot;\nconst short* bonus = &effect->bonus;\n"
        "effect[1].bonus = *bonus;",
        "effect[1].bonus = effect->bonus;\n"
        "effect[1].dwelling = slot;")
    if not contract_violations(reordered_initialize_hordes,
                               initialize_hordes_key):
        failures.append(
            "reordered initialize_hordes dwelling/bonus stores passed")
    give_spells_key = ("town.obj", 0x1665A0)
    give_spells_probe = """\
if (currentHero) {
    if (currentHero->IsWieldingArtifact(ARTIFACT_SPELLBOOK)) {
        if (HasBuilding(MAGE_GUILD_ID, 1)) {
            grant_spells();
        }
    }
}
"""
    if contract_violations(give_spells_probe, give_spells_key):
        failures.append("aligned GiveSpells nested guards did not pass")
    flattened_give_spells = """\
if (currentHero
    && currentHero->IsWieldingArtifact(ARTIFACT_SPELLBOOK)
    && HasBuilding(MAGE_GUILD_ID, 1)) {
    grant_spells();
}
"""
    if not contract_violations(flattened_give_spells, give_spells_key):
        failures.append("flattened GiveSpells guard scopes passed")
    swap_heroes_key = ("town.obj", 0x166864)
    swap_heroes_probe = """\
hero* garrisonHero = gpGame->GetHero(currentTown->garrisonHeroId);
hero* visitingHero = gpGame->GetHero(currentTown->visitingHeroId);
std::swap(currentTown->garrisonHeroId, currentTown->visitingHeroId);
int rosterIndex = gpCurrentPlayer->FindHero(visitingHero->id);
gpGame->record_hide_hero(visitingHero, visitingHero->owner, 0);
visitingHero->restore_cell();
CMCHideHero hideHero(visitingHero->id);
SendMapChange(&hideHero);
for (int i = rosterIndex; i < gpCurrentPlayer->numHeroes - 1; ++i)
    gpCurrentPlayer->heroes[i] = gpCurrentPlayer->heroes[i + 1];
--gpCurrentPlayer->numHeroes;
gpCurrentPlayer->heroes[gpCurrentPlayer->numHeroes] = -1;
if (gpCurrentPlayer->currHeroId == visitingHero->id) {
    gpCurrentPlayer->currHeroId = -1;
    if (gNetLocalGamePos == visitingHero->owner) {
        gpAdvManager->drawCursor = 0;
        gpAdvManager->inDialog = 0;
    }
}
hero* placedHero = gpGame->GetHero(garrisonHero->id);
point.x = currentTown->mapX;
point.y = currentTown->mapY;
point.z = currentTown->mapZ;
placedHero->PlaceInMap(player, point, 0);
"""
    if contract_violations(swap_heroes_probe, swap_heroes_key):
        failures.append("aligned SwapHeroes source shape did not pass")
    broken_swap_heroes_probes = (
        (swap_heroes_probe.replace(
            "hero* garrisonHero = gpGame->GetHero(currentTown->garrisonHeroId);\n"
            "hero* visitingHero = gpGame->GetHero(currentTown->visitingHeroId);",
            "hero* visitingHero = gpGame->GetHero(currentTown->visitingHeroId);\n"
            "hero* garrisonHero = gpGame->GetHero(currentTown->garrisonHeroId);"),
         "two resident GetHero statements"),
        (swap_heroes_probe.replace(
            "record_hide_hero", "GameFn_0049C720"),
         "record_hide_hero, restore_cell"),
        (swap_heroes_probe.replace(
            "--gpCurrentPlayer->numHeroes;\n",
            "", 1).replace(
                "for (int i = rosterIndex;",
                "--gpCurrentPlayer->numHeroes;\nfor (int i = rosterIndex;"),
         "roster-shift loop before the count decrement"),
        (swap_heroes_probe.replace(
            "        gpAdvManager->inDialog = 0;\n", ""),
         "nested current/local-owner latch scope"),
    )
    for probe, description in broken_swap_heroes_probes:
        if not any(description in rule.description for rule in
                   contract_violations(probe, swap_heroes_key)):
            failures.append("broken SwapHeroes " + description
                            + " source shape passed")
    is_capitol_key = ("game.obj", 0xBCCB4)
    is_capitol_probe = "return HasBuilding(HALL_CAPITOL_ID, 0);"
    if contract_violations(is_capitol_probe, is_capitol_key):
        failures.append("aligned IsCapitol helper body did not pass")
    flattened_is_capitol = "return (built & bitNumber[HALL_CAPITOL_ID]) != 0;"
    if not contract_violations(flattened_is_capitol, is_capitol_key):
        failures.append("flattened IsCapitol helper body passed")
    set_spells_key = ("town.obj", 0x166B64)
    set_spells_probe = """\
memset(mageGuildSpellCounts, 0, sizeof(mageGuildSpellCounts));
for (int level = 1; level <= field_14; level++) {
    int count = gMageGuildBaseSpellCounts[level - 1];
    if (type == TOWN_TOWER && HasBuilding(EXTRA_1_ID, 1))
        count++;
    while (count > 0 && mageGuildSpells[level - 1][count - 1] == -1)
        count--;
    mageGuildSpellCounts[level - 1] = count;
}
"""
    if contract_violations(set_spells_probe, set_spells_key):
        failures.append("aligned set_spells_available body did not pass")
    hoisted_spell_count = set_spells_probe.replace(
        "for (int level = 1; level <= field_14; level++) {\n"
        "    int count = gMageGuildBaseSpellCounts[level - 1];",
        "int count;\nfor (int level = 1; level <= field_14; level++) {\n"
        "    count = gMageGuildBaseSpellCounts[level - 1];")
    if not contract_violations(hoisted_spell_count, set_spells_key):
        failures.append("hoisted set_spells_available count local passed")
    flattened_library_test = set_spells_probe.replace(
        "HasBuilding(EXTRA_1_ID, 1)",
        "(active & bitNumber[EXTRA_1_ID])")
    if not contract_violations(flattened_library_test, set_spells_key):
        failures.append("flattened set_spells_available HasBuilding passed")
    destroy_capitol_key = ("town.obj", 0x166ED8)
    destroy_capitol_probe = """\
if (IsCapitol() && owner >= 0) {
    town* other_town = gpGame->GetTown(town_id);
    if (other_town->IsCapitol()) {
        built &= ~bitNumber[HALL_CAPITOL_ID];
        built |= bitNumber[HALL_CITY_ID];
        active &= ~bitNumber[HALL_CAPITOL_ID];
        NewmapCell* cell = gpGame->worldMap.cell(mapX, mapY, mapZ);
        gpGame->ConvertObject(cell);
    }
}
"""
    if contract_violations(destroy_capitol_probe, destroy_capitol_key):
        failures.append("aligned destroy_extra_capitol shape did not pass")
    flattened_self_capitol = destroy_capitol_probe.replace(
        "IsCapitol() && owner >= 0",
        "(built & bitNumber[HALL_CAPITOL_ID]) && owner >= 0")
    if not any("both Dreamcast IsCapitol" in rule.description for rule in
               contract_violations(flattened_self_capitol,
                                   destroy_capitol_key)):
        failures.append("flattened destroy_extra_capitol self helper passed")
    reordered_capitol_masks = destroy_capitol_probe.replace(
        "built &= ~bitNumber[HALL_CAPITOL_ID];\n"
        "        built |= bitNumber[HALL_CITY_ID];",
        "built |= bitNumber[HALL_CITY_ID];\n"
        "        built &= ~bitNumber[HALL_CAPITOL_ID];")
    if not any("three mask updates in order" in rule.description for rule in
               contract_violations(reordered_capitol_masks,
                                   destroy_capitol_key)):
        failures.append("reordered destroy_extra_capitol masks passed")
    flattened_capitol_cell = destroy_capitol_probe.replace(
        "gpGame->worldMap.cell(mapX, mapY, mapZ)",
        "&gpGame->worldMap.cellData[index]")
    if not any("NewfullMap::cell boundary" in rule.description for rule in
               contract_violations(flattened_capitol_cell,
                                   destroy_capitol_key)):
        failures.append("flattened destroy_extra_capitol cell helper passed")
    build_building_key = ("town.obj", 0x166FC8)
    build_building_probe = """\
type_building_id built;
unsigned char had_fort = IsCastle();
unsigned char had_capitol = IsCapitol();
built = create_building(type_building_id(buildingId));
update_full_building_mask();
set_spells_available();
set_spells_available();
GiveSpells(0);
if ((IsCapitol() && !had_capitol) || (IsCastle() && !had_fort)) {
    refresh();
}
"""
    if contract_violations(build_building_probe, build_building_key):
        failures.append("aligned BuildBuilding source shape did not pass")
    renamed_built = build_building_probe.replace(
        "type_building_id built;", "type_building_id result;").replace(
        "built = create_building", "result = create_building")
    if not contract_violations(renamed_built, build_building_key):
        failures.append("renamed BuildBuilding raw local passed")
    flattened_full_mask = build_building_probe.replace(
        "update_full_building_mask();", "active = built;")
    if not contract_violations(flattened_full_mask, build_building_key):
        failures.append("flattened BuildBuilding full-mask helper passed")
    missing_spell_refresh = build_building_probe.replace(
        "set_spells_available();\nset_spells_available();",
        "set_spells_available();")
    if not contract_violations(missing_spell_refresh, build_building_key):
        failures.append("single BuildBuilding spell refresh passed")
    flattened_change_test = build_building_probe.replace(
        "IsCapitol() && !had_capitol", "HasBuilding(HALL_CAPITOL_ID, 0)")
    if not contract_violations(flattened_change_test, build_building_key):
        failures.append("flattened BuildBuilding later helper passed")
    full_mask_key = ("town.obj", 0x168494)
    full_mask_probe = """\
active = built;
for (int i = 0; i < MAX_BUILDING_TYPE; i++) {
    if (HasBuilding(i, 0))
        active |= included_buildings[type][i];
}
"""
    if contract_violations(full_mask_probe, full_mask_key):
        failures.append("aligned update_full_building_mask body did not pass")
    flattened_full_mask_test = full_mask_probe.replace(
        "HasBuilding(i, 0)", "built & bitNumber[i]")
    if not contract_violations(flattened_full_mask_test, full_mask_key):
        failures.append("flattened update_full_building_mask helper passed")
    grail_win_key = ("victorylossconditions.obj", 0x190124)
    grail_win_probe = """\
type_point any_town_loc(-1, -1, -1);
type_point grail_town_loc(TownX, TownY, TownZ);
if (gpGame->OnSameTeam(player, gNetLocalGamePos)) {
    type_point this_town_loc(thisTown->mapX, thisTown->mapY, thisTown->mapZ);
    if (this_town_loc == grail_town_loc
        || grail_town_loc == any_town_loc) {
        if (thisTown->HasBuilding(HOLY_GRAIL_ID, 1)) {
            win();
        }
    }
}
"""
    if contract_violations(grail_win_probe, grail_win_key):
        failures.append("aligned Grail victory source shape did not pass")
    grail_win_mutations = (
        grail_win_probe.replace("grail_town_loc", "target"),
        grail_win_probe.replace(
            "gpGame->OnSameTeam(player, gNetLocalGamePos)",
            "same_team(gpGame, player, gNetLocalGamePos)"),
        grail_win_probe.replace(
            "this_town_loc == grail_town_loc",
            "this_town_loc.operator==(&grail_town_loc)"),
        grail_win_probe.replace(
            "thisTown->HasBuilding(HOLY_GRAIL_ID, 1)",
            "thisTown->active & bitNumber[HOLY_GRAIL_ID]"),
    )
    if any(not contract_violations(probe, grail_win_key)
           for probe in grail_win_mutations):
        failures.append("flattened Grail victory source shape passed")
    area_effect_key = ("spells.obj", 0x153B60)
    area_effect_probe = """\
hero* casting_hero;
unsigned char multiple_targets;
SpellEffect(akSpellTraits[iSpellType].m_effect, targetCell, 100, 0);
std::vector<army*> targets;
long damage;
mark_area_effect(iSpellType, targetCell, mastery, targets);
casting_hero = heroes[currentSide];
multiple_targets = 0;
effected[target->combatSide][target->bitIndex] = 0;
damage = ComputeSpellDamage(iSpellType, power, mastery, casting_hero,
                            target->get_controller(), target, 0);
if (!victim)
    victim = target;
else
    multiple_targets = 1;
if (victim) {
    if (multiple_targets) {
        damage = ComputeSpellDamage(iSpellType, power, mastery,
                                    casting_hero, 0, 0, 0);
    }
}
"""
    if contract_violations(area_effect_probe, area_effect_key):
        failures.append("aligned AreaEffect source shape did not pass")
    reordered_area_effect_locals = area_effect_probe.replace(
        "std::vector<army*> targets;\nlong damage;",
        "long damage;\nstd::vector<army*> targets;")
    if not any("declaration order" in rule.description for rule in
               contract_violations(reordered_area_effect_locals,
                                   area_effect_key)):
        failures.append("reordered AreaEffect locals passed")
    reordered_area_effect_setup = area_effect_probe.replace(
        "SpellEffect(akSpellTraits[iSpellType].m_effect, targetCell, 100, 0);\n"
        "std::vector<army*> targets;\nlong damage;\n"
        "mark_area_effect(iSpellType, targetCell, mastery, targets);",
        "std::vector<army*> targets;\nlong damage;\n"
        "mark_area_effect(iSpellType, targetCell, mastery, targets);\n"
        "SpellEffect(akSpellTraits[iSpellType].m_effect, targetCell, 100, 0);")
    if not any("statement order" in rule.description for rule in
               contract_violations(reordered_area_effect_setup,
                                   area_effect_key)):
        failures.append("reordered AreaEffect setup statements passed")
    flattened_area_effect_caster = area_effect_probe.replace(
        "casting_hero, 0, 0, 0", "heroes[currentSide], 0, 0, 0")
    if not any("both Dreamcast ComputeSpellDamage" in rule.description
               for rule in contract_violations(flattened_area_effect_caster,
                                                area_effect_key)):
        failures.append("flattened AreaEffect casting_hero use passed")
    renamed_area_effect_multiple = area_effect_probe.replace(
        "multiple_targets", "several")
    if not any("multiple_targets initialization" in rule.description
               for rule in contract_violations(renamed_area_effect_multiple,
                                                area_effect_key)):
        failures.append("renamed AreaEffect multiple_targets local passed")
    erased_area_effect_clear = area_effect_probe.replace(
        "effected[target->combatSide][target->bitIndex] = 0;\n", "")
    if not any("effected clear" in rule.description for rule in
               contract_violations(erased_area_effect_clear,
                                   area_effect_key)):
        failures.append("erased AreaEffect effected clear passed")
    memory_sample_key = ("soundmgr.obj", 0x14B528)
    memory_sample_probe = """\
slot = range->next++;
if (range->next >= range->last) {
    slot = range->next = range->first;
}
StopSample(sampleHandles[slot]);
if (gUnk698764)
    AIL_set_sample_volume(handle, ConvertVolume(sPtr->field_2c, 100));
else
    AIL_set_sample_volume(handle, 0);
AIL_start_sample(handle);
sPtr->field_1c = handle;
LeaveCriticalSection(&section_sound_call);
gpSoundManager->service_sounds();
return handle;
"""
    if contract_violations(memory_sample_probe, memory_sample_key):
        failures.append("aligned MemorySample source shape did not pass")
    split_memory_sample_wrap = memory_sample_probe.replace(
        "slot = range->next = range->first;",
        "slot = range->first;\n    range->next = range->first;")
    if not any("one-statement wrapped" in rule.description for rule in
               contract_violations(split_memory_sample_wrap,
                                   memory_sample_key)):
        failures.append("split MemorySample wrap assignment passed")
    expanded_memory_sample_stop = memory_sample_probe.replace(
        "StopSample(sampleHandles[slot]);",
        "AIL_end_sample(sampleHandles[slot]);")
    if not any("StopSample helper boundary" in rule.description for rule in
               contract_violations(expanded_memory_sample_stop,
                                   memory_sample_key)):
        failures.append("expanded MemorySample StopSample passed")
    reordered_memory_sample_start = memory_sample_probe.replace(
        "AIL_start_sample(handle);", "").replace(
            "if (gUnk698764)", "AIL_start_sample(handle);\nif (gUnk698764)")
    if not any("statement order" in rule.description for rule in
               contract_violations(reordered_memory_sample_start,
                                   memory_sample_key)):
        failures.append("reordered MemorySample start passed")
    duplicated_memory_sample_volume = memory_sample_probe.replace(
        "AIL_set_sample_volume(handle, 0);",
        "AIL_set_sample_volume(handle, ConvertVolume(0, 100));")
    if not any("exactly one Dreamcast ConvertVolume" in rule.description
               for rule in contract_violations(
                   duplicated_memory_sample_volume, memory_sample_key)):
        failures.append("duplicated MemorySample ConvertVolume passed")
    flattened_memory_sample_service = memory_sample_probe.replace(
        "gpSoundManager->service_sounds();", "ServeSampleStream();")
    if not any("named service_sounds boundary" in rule.description
               for rule in contract_violations(
                   flattened_memory_sample_service, memory_sample_key)):
        failures.append("flattened MemorySample service_sounds passed")
    launch_sample_key = ("soundmgr.obj", 0x14B780)
    launch_sample_probe = """\
launched->sample2.playSample =
    gpSoundManager->MemorySample(launched->sample2.resSample);
gpSoundManager->service_sounds();
if (!bShutDownDone)
    _beginthread(WaitEndSampleThread, 0, launched);
"""
    if contract_violations(launch_sample_probe, launch_sample_key):
        failures.append("aligned launch_sample helper shape did not pass")
    flattened_launch_sample_service = launch_sample_probe.replace(
        "gpSoundManager->service_sounds();", "ServeSampleStream();")
    launch_service_violations = contract_violations(
        flattened_launch_sample_service, launch_sample_key)
    if not any("MemorySample then service_sounds" in rule.description
               for rule in launch_service_violations):
        failures.append("flattened launch_sample service order passed")
    if not any("exactly one named Complete" in rule.description
               for rule in launch_service_violations):
        failures.append("flattened launch_sample service count passed")
    palette24_key = ("resourcemanager.obj", 0x121EC8)
    palette24_path = """\
streamInterface->Read(header, sizeof(header));
streamInterface->Read(rgba, sizeof(rgba));
result = new TPalette24(rgba);
if (gGraphicsSaturated)
    result->AdjustHSV(-1.0f, -1.0f, 1.5f, 1.2f);
"""
    palette24_probe = """\
TPalette24* result;
char header[24];
TRGBA rgba[256];
""" + palette24_path * 2
    if contract_violations(palette24_probe, palette24_key):
        failures.append("aligned GetPalette24 source shape did not pass")
    renamed_palette24 = palette24_probe.replace("rgba", "paletteData")
    if not any("recovered names" in rule.description for rule in
               contract_violations(renamed_palette24, palette24_key)):
        failures.append("renamed GetPalette24 rgba local passed")
    reversed_palette24 = palette24_probe.replace(
        "streamInterface->Read(header, sizeof(header));\n"
        "streamInterface->Read(rgba, sizeof(rgba));",
        "streamInterface->Read(rgba, sizeof(rgba));\n"
        "streamInterface->Read(header, sizeof(header));", 1)
    if not any("header-then-rgba" in rule.description for rule in
               contract_violations(reversed_palette24, palette24_key)):
        failures.append("reversed GetPalette24 read sequence passed")
    flattened_palette24 = palette24_probe.replace(
        "result = new TPalette24(rgba);",
        "result = NewPalette24(rgba);", 1)
    if not any("direct TPalette24 construction" in rule.description
               for rule in contract_violations(flattened_palette24,
                                                palette24_key)):
        failures.append("flattened GetPalette24 constructor passed")
    resource_display_key = ("resourcedisplay.obj", 0x120C54)
    resource_display_probe = """\
if (isSmall) {
    initialize(7, 0x23f, 0x2e2, 0x16, parent);
    resourceBackground = new bitmapBorder(
        0, 0, 0x2e2, 0x16, 1000, "kresbar.pcx", 0x800);
} else {
    initialize(3, 0x23f, 0x31a, 0x16, parent);
    resourceBackground = new bitmapBorder(
        0, 0, 0x31a, 0x16, 1000, "aresbar.pcx", 0x800);
}
resourceBackground->SetPlayerPaletteColors(
    gpGame->GetLocalPlayerGamePos());
AddWidget(resourceBackground, -1);
for (int i = 0; i < NUM_RESOURCES; ++i) {
    resourceWidgets[i] = new textWidget(textX, i);
    AddWidget(resourceWidgets[i], -1);
    resourceBorders[i] = new border(textX, i);
    AddWidget(resourceBorders[i], -1);
    textX += spacing;
}
if (static_cast<volatile unsigned char&>(is_small)) {
    statusWidget = new textWidget(0x22b, 3);
} else {
    statusWidget = new textWidget(0x25f, 3);
}
AddWidget(statusWidget, -1);
"""
    if contract_violations(resource_display_probe, resource_display_key):
        failures.append(
            "aligned TResourceDisplay constructor shape did not pass")
    reordered_resource_background = resource_display_probe.replace(
        "    initialize(7, 0x23f, 0x2e2, 0x16, parent);\n"
        "    resourceBackground = new bitmapBorder(\n"
        "        0, 0, 0x2e2, 0x16, 1000, \"kresbar.pcx\", 0x800);",
        "    resourceBackground = new bitmapBorder(\n"
        "        0, 0, 0x2e2, 0x16, 1000, \"kresbar.pcx\", 0x800);\n"
        "    initialize(7, 0x23f, 0x2e2, 0x16, parent);")
    if not any("paired size arms" in rule.description for rule in
               contract_violations(reordered_resource_background,
                                   resource_display_key)):
        failures.append(
            "reordered TResourceDisplay background arm passed")
    flattened_resource_loop = resource_display_probe.replace(
        "    AddWidget(resourceWidgets[i], -1);\n"
        "    resourceBorders[i] = new border(textX, i);",
        "    resourceBorders[i] = new border(textX, i);\n"
        "    AddWidget(resourceWidgets[i], -1);")
    if not any("seven-resource" in rule.description for rule in
               contract_violations(flattened_resource_loop,
                                   resource_display_key)):
        failures.append(
            "reordered TResourceDisplay construction loop passed")
    flattened_resource_status = resource_display_probe.replace(
        "    statusWidget = new textWidget(0x22b, 3);",
        "    statusWidget = new textWidget(0x22b, 3);\n"
        "    AddWidget(statusWidget, -1);").replace(
        "    statusWidget = new textWidget(0x25f, 3);",
        "    statusWidget = new textWidget(0x25f, 3);\n"
        "    AddWidget(statusWidget, -1);").replace(
        "}\nAddWidget(statusWidget, -1);", "}")
    if not any("shared AddWidget" in rule.description for rule in
               contract_violations(flattened_resource_status,
                                   resource_display_key)):
        failures.append(
            "flattened TResourceDisplay status-widget arms passed")
    recruit_update_key = ("recruit.obj", 0x119DCC)
    recruit_update_probe = """\
message msg;
if (slot == -1)
    slot = selectedPosition;
UpdateCost();
sprintf(gText, "%s %s",
    (*gpGeneralText)[GENERAL_TEXT_RECRUIT_TITLE],
    GetArmyName(monsterType, 2));
long maxGold = gpCurrentPlayer->resources[6] / goldPerTroop;
if (altResource != -1) {
    int byResource = resources[altResource] / resourcesPerTroop;
    maxAvail = maxGold < byResource ? maxGold : byResource;
} else {
    maxAvail = maxGold;
}
"""
    if contract_violations(recruit_update_probe, recruit_update_key):
        failures.append("aligned recruitUnit::Update source shape did not pass")
    broken_recruit_update_probes = (
        (recruit_update_probe.replace(
            "message msg;", "message msg = {0, 0, 0, 0, 0, 0, 0, 0};"),
         "root message constructor"),
        (recruit_update_probe.replace(
            "(*gpGeneralText)[GENERAL_TEXT_RECRUIT_TITLE]",
            "gpGeneralText->GetText(GENERAL_TEXT_RECRUIT_TITLE)"),
         "one-statement"),
        (recruit_update_probe.replace(
            "GetArmyName(monsterType, 2)",
            "akCreatureTypeTraits[monsterType].m_plural_name"),
         "flatten Dreamcast's GetArmyName"),
        (recruit_update_probe.replace("long maxGold", "int maxGold"),
         "long maxGold"),
    )
    for probe, description in broken_recruit_update_probes:
        if not any(description in rule.description for rule in
                   contract_violations(probe, recruit_update_key)):
            failures.append("broken recruitUnit::Update " + description
                            + " source shape passed")
    update_cost_key = ("recruit.obj", 0x11AC7C)
    update_cost_probe = """\
int resCost[7];
GetMonsterCost(monsterType, resCost);
goldPerTroop = resCost[6];
int i;
for (i = 0; i < 6; i++) {
    if (resCost[i])
        break;
}
"""
    if contract_violations(update_cost_probe, update_cost_key):
        failures.append("aligned recruitUnit::UpdateCost shape did not pass")
    renamed_update_cost = update_cost_probe.replace("resCost", "cost")
    if not any("sole resCost local" in rule.description for rule in
               contract_violations(renamed_update_cost, update_cost_key)):
        failures.append("renamed UpdateCost resCost local passed")
    flattened_update_cost = update_cost_probe.replace(
        "GetMonsterCost(monsterType, resCost);",
        "memcpy(resCost, &gCreatureRecords[monsterType * 29], "
        "sizeof(resCost));")
    flattened_rules = contract_violations(flattened_update_cost,
                                           update_cost_key)
    if not any("GetMonsterCost helper boundary" in rule.description
               for rule in flattened_rules):
        failures.append("flattened UpdateCost helper boundary passed")
    if not any("duplicated record-table" in rule.description
               for rule in flattened_rules):
        failures.append("flattened UpdateCost direct record access passed")
    recruit_contract_source = """\
#define HOMM3_RECRUIT_MESSAGE_CTOR_VIEW
#include "message.h"
message msg;
inline void recruitUnit::UpdateCost()
{
    int resCost[7];
    GetMonsterCost(monsterType, resCost);
}
"""
    recruit_contract_header = "inline void UpdateCost();\n"
    recruit_message_header = """\
#if defined(HOMM3_RECRUIT_MESSAGE_CTOR_VIEW)
message() {
    id = 0; codeX = 0; codeY = 0; qualifier = 0;
    mouseX = 0; mouseY = 0; extra = 0; window = 0;
}
#endif
"""
    if recruit_inline_contract_violations(
            recruit_contract_source, recruit_contract_header,
            recruit_message_header):
        failures.append("aligned recruit inline contracts did not pass")
    broken_recruit_inline_probes = (
        recruit_contract_source.replace(
            "#define HOMM3_RECRUIT_MESSAGE_CTOR_VIEW\n", ""),
        recruit_contract_source.replace(
            "inline void recruitUnit::UpdateCost()",
            "void recruitUnit::UpdateCost()"),
    )
    for probe in broken_recruit_inline_probes:
        if not recruit_inline_contract_violations(
                probe, recruit_contract_header, recruit_message_header):
            failures.append("broken recruit inline source contract passed")
    if not recruit_inline_contract_violations(
            recruit_contract_source, recruit_contract_header,
            recruit_message_header.replace(
                "HOMM3_RECRUIT_MESSAGE_CTOR_VIEW",
                "HOMM3_OTHER_MESSAGE_CTOR_VIEW")):
        failures.append("missing recruit message constructor guard passed")
    set_hero_context_key = ("advmgr.obj", 0x1A878)
    set_hero_context_probe = """\
playerData* player = gpCurrentPlayer;
if (waitingPlayer)
    player = gpGame->GetLocalPlayer();
else
    inDialog = 1;
player->currHeroId = heroId;
hero* curr = &gpGame->heroes[heroId];
NewmapCell* cell;
int found = player->FindHero(heroId);
if (found == -1)
    found = 0;
advWindow->UpdateHeroLocators(found, draw_changes, 0);
seedingValid = 0;
if (curr->pathTargetX >= 0) {
    type_point routeTarget = curr->get_target();
    SeedTo(routeTarget);
}
ShowRoute(0, 0, 1);
if (cell->GroundSet != field_58 && draw_changes) {
    field_58 = cell->GroundSet;
    gpSoundManager->SwitchAmbientMusic(gTerrainMusicIds[field_58]);
}
"""
    if contract_violations(set_hero_context_probe, set_hero_context_key):
        failures.append("aligned SetHeroContext shape did not pass")
    broken_set_hero_context_probes = (
        (set_hero_context_probe.replace("hero* curr =", "hero* activeHero ="),
         "player, curr and cell local identities"),
        (set_hero_context_probe.replace("int found =", "int heroSlot ="),
         "found local around FindHero"),
        (set_hero_context_probe.replace(
            "type_point routeTarget = curr->get_target();",
            "type_point routeTarget(curr->pathTargetX, curr->pathTargetY, "
            "curr->pathTargetZ);"),
         "get_target helper boundary"),
        (set_hero_context_probe.replace(
            "if (cell->GroundSet != field_58 && draw_changes) {\n"
            "    field_58 = cell->GroundSet;",
            "if (fullMap->cellData->GroundSet != field_58 && draw_changes) "
            "{\n    field_58 = fullMap->cellData->GroundSet;"),
         "cell local live through the tail"),
    )
    for probe, description in broken_set_hero_context_probes:
        if not any(description in rule.description for rule in
                   contract_violations(probe, set_hero_context_key)):
            failures.append("broken SetHeroContext " + description
                            + " source shape passed")
    show_boat_ctor_key = ("event_record.obj", 0x8CFA8)
    show_boat_ctor_probe = """\
previous_location = _current_boat->get_location();
location = _location;
"""
    if contract_violations(show_boat_ctor_probe, show_boat_ctor_key):
        failures.append("aligned type_record_show_boat shape did not pass")
    flattened_show_boat_location = show_boat_ctor_probe.replace(
        "_current_boat->get_location()",
        "type_point(_current_boat->x, _current_boat->y, "
        "_current_boat->z)")
    if not any("get_location helper" in rule.description for rule in
               contract_violations(flattened_show_boat_location,
                                   show_boat_ctor_key)):
        failures.append("flattened show-boat get_location helper passed")
    reordered_show_boat_location = """\
location = _location;
previous_location = _current_boat->get_location();
"""
    if not any("before the destination" in rule.description for rule in
               contract_violations(reordered_show_boat_location,
                                   show_boat_ctor_key)):
        failures.append("reordered show-boat location statements passed")
    record_show_boat_key = ("event_record.obj", 0x8E18C)
    record_show_boat_probe = """\
eventRecords.push_back(
    new type_record_show_boat(current_boat, point));
"""
    if contract_violations(record_show_boat_probe, record_show_boat_key):
        failures.append("aligned record_show_boat shape did not pass")
    split_record_show_boat = """\
type_record_show_boat* record =
    new type_record_show_boat(current_boat, point);
eventRecords.push_back(record);
"""
    if not any("inside one eventRecords push_back" in rule.description
               for rule in contract_violations(split_record_show_boat,
                                                record_show_boat_key)):
        failures.append("split record_show_boat construction passed")
    move_record_ctor_key = ("event_record.obj", 0x8C710)
    move_record_ctor_probe = """\
current_hero = _hero;
restore_flag = _hero->facing;
direction = _direction;
source = _hero->get_location();
destination = _destination;
"""
    if contract_violations(move_record_ctor_probe, move_record_ctor_key):
        failures.append("aligned move-record constructor shape did not pass")
    broken_move_record_probes = (
        move_record_ctor_probe.replace(
            "_hero->get_location()", "type_point(_hero->x, _hero->y, _hero->z)"),
        move_record_ctor_probe.replace(
            "direction = _direction;\nsource = _hero->get_location();",
            "source = _hero->get_location();\ndirection = _direction;"),
    )
    if any(not contract_violations(probe, move_record_ctor_key)
           for probe in broken_move_record_probes):
        failures.append("broken move-record constructor source shape passed")
    record_teleport_key = ("event_record.obj", 0x8E2F8)
    record_teleport_probe = """\
eventRecords.push_back(new type_record_teleport(who, destination));
"""
    if contract_violations(record_teleport_probe, record_teleport_key):
        failures.append("aligned record_teleport shape did not pass")
    split_record_teleport = """\
type_record_teleport* record =
    new type_record_teleport(who, destination);
eventRecords.push_back(record);
"""
    if not any("inside one eventRecords push_back" in rule.description
               for rule in contract_violations(split_record_teleport,
                                                record_teleport_key)):
        failures.append("split record_teleport construction passed")
    claim_mine_ctor_key = ("event_record.obj", 0x8CB2C)
    claim_mine_ctor_probe = """\
id = _id;
new_owner = _new_owner;
old_owner = gpGame->mines[_id].playerOwner;
"""
    if contract_violations(claim_mine_ctor_probe, claim_mine_ctor_key):
        failures.append("aligned claim-mine constructor body did not pass")
    if not contract_violations(claim_mine_ctor_probe.replace(
            "id = _id;\nnew_owner = _new_owner;",
            "new_owner = _new_owner;\nid = _id;"), claim_mine_ctor_key):
        failures.append("reordered claim-mine constructor body passed")
    claim_town_ctor_key = ("event_record.obj", 0x8CCFC)
    claim_town_ctor_probe = """\
id = _id;
new_owner = _new_owner;
old_owner = gpGame->towns[_id].owner;
"""
    if contract_violations(claim_town_ctor_probe, claim_town_ctor_key):
        failures.append("aligned claim-town constructor body did not pass")
    flattened_claim_town = claim_town_ctor_probe.replace(
        "gpGame->towns[_id].owner", "gpGame->mines[_id].playerOwner")
    if not contract_violations(flattened_claim_town, claim_town_ctor_key):
        failures.append("wrong claim-town owner snapshot passed")
    record_claim_mine_key = ("event_record.obj", 0x8DFE0)
    record_claim_mine_probe = """\
mine& current_mine = mines[id];
type_point location(current_mine.mapX, current_mine.mapY, current_mine.mapZ);
CMCClaimMine msg(id, new_owner);
SendMapChange(&msg);
eventRecords.push_back(new type_record_claim_mine(id, new_owner));
"""
    if contract_violations(record_claim_mine_probe, record_claim_mine_key):
        failures.append("aligned record_claim_mine shape did not pass")
    broken_record_claim_mine_probes = (
        record_claim_mine_probe.replace(
            "mine& current_mine = mines[id];\n", ""),
        record_claim_mine_probe.replace(
            "mine& current_mine = mines[id];\n"
            "type_point location(current_mine.mapX, current_mine.mapY, "
            "current_mine.mapZ);",
            "type_point location(mines[id].mapX, mines[id].mapY, "
            "mines[id].mapZ);"),
        record_claim_mine_probe.replace("current_mine.mapZ",
                                        "current_mine.mapY"),
        record_claim_mine_probe.replace(
            "CMCClaimMine msg(id, new_owner);\nSendMapChange(&msg);",
            "SendMapChange(&msg);\nCMCClaimMine msg(id, new_owner);"),
        record_claim_mine_probe.replace(
            "eventRecords.push_back(new type_record_claim_mine(id, "
            "new_owner));",
            "type_record_claim_mine* record = new "
            "type_record_claim_mine(id, new_owner);\n"
            "eventRecords.push_back(record);"),
    )
    if any(not contract_violations(probe, record_claim_mine_key)
           for probe in broken_record_claim_mine_probes):
        failures.append("broken record_claim_mine source shape passed")
    retail_only_record_claim_mine = (
        "GetMine(id);\n" + record_claim_mine_probe)
    if contract_violations(retail_only_record_claim_mine,
                           record_claim_mine_key):
        failures.append(
            "record_claim_mine asymmetric rules rejected an extra statement")
    record_claim_town_key = ("event_record.obj", 0x8E058)
    record_claim_town_probe = """\
GetTown(id);
CMCClaimTown msg(id, new_owner);
SendMapChange(&msg);
eventRecords.push_back(new type_record_claim_town(id, new_owner));
"""
    if contract_violations(record_claim_town_probe, record_claim_town_key):
        failures.append("aligned record_claim_town shape did not pass")
    broken_record_claim_town_probes = (
        record_claim_town_probe.replace("GetTown(id);\n", ""),
        record_claim_town_probe.replace(
            "CMCClaimTown msg(id, new_owner);\nSendMapChange(&msg);",
            "SendMapChange(&msg);\nCMCClaimTown msg(id, new_owner);"),
        record_claim_town_probe.replace(
            "eventRecords.push_back(new type_record_claim_town(id, "
            "new_owner));",
            "type_record_claim_town* record = new "
            "type_record_claim_town(id, new_owner);\n"
            "eventRecords.push_back(record);"),
    )
    if any(not contract_violations(probe, record_claim_town_key)
           for probe in broken_record_claim_town_probes):
        failures.append("broken record_claim_town source shape passed")
    event_record_ctor_probe = r"""
// E:\gamedcs\event_record.cpp:96
inline type_record_move_hero::type_record_move_hero(
    hero* _hero, char _direction, type_point _destination)
{
}
// E:\gamedcs\event_record.cpp:204
inline type_record_teleport::type_record_teleport(
    hero* _hero, type_point _destination)
    : type_record_move_hero(_hero, _hero->facing, _destination)
{
}
// E:\gamedcs\event_record.cpp:211
// E:\gamedcs\event_record.cpp:237
inline type_record_claim_mine::type_record_claim_mine(
    long _id, char _new_owner)
{
}
// E:\gamedcs\event_record.cpp:247
// E:\gamedcs\event_record.cpp:321
inline type_record_claim_town::type_record_claim_town(
    long _id, char _new_owner)
    : type_record_claim_mine()
{
}
// E:\gamedcs\event_record.cpp:331
"""
    if event_record_constructor_violations(event_record_ctor_probe):
        failures.append("aligned event-record constructor sites did not pass")
    broken_event_record_ctor_probes = (
        event_record_ctor_probe.replace(
            "inline type_record_move_hero::type_record_move_hero(",
            "__forceinline type_record_move_hero::type_record_move_hero("),
        event_record_ctor_probe.replace(
            ": type_record_move_hero(_hero, _hero->facing, _destination)",
            ""),
        event_record_ctor_probe.replace(
            "inline type_record_claim_mine::type_record_claim_mine(",
            "__forceinline type_record_claim_mine::type_record_claim_mine("),
        event_record_ctor_probe.replace(": type_record_claim_mine()", ""),
    )
    if any(not event_record_constructor_violations(probe)
           for probe in broken_event_record_ctor_probes):
        failures.append("broken event-record constructor site passed")
    combat_monster_key = ("events.obj", 0x9AF34)
    combat_monster_probe = """\
DemobilizeCurrHero(0, 1);
int event_seed = point.x * 0x3c907;
SRand(event_seed);
who->army.get_AI_value();
who->get_primary_skill_total();
double ratio = who->army.get_AI_value();
ratio /= combat_value;
armyGroup army_group;
if (monType2 != CREATURE_NONE || monType3 != CREATURE_NONE) {
    int tempNumTroops[7];
    TCreatureType tempArmies[7];
}
"""
    if contract_violations(combat_monster_probe, combat_monster_key):
        failures.append("aligned CombatMonsterEvent shape did not pass")
    broken_combat_monster_probes = (
        (combat_monster_probe.replace(
            "int event_seed = point.x * 0x3c907;\nSRand(event_seed);",
            "int seed = point.x * 0x3c907;\nSRand(seed);"),
         "event_seed local"),
        (combat_monster_probe.replace(
            "who->get_primary_skill_total();\n"
            "double ratio = who->army.get_AI_value();",
            "double ratio = who->army.get_AI_value();\n"
            "who->get_primary_skill_total();"),
         "primary-skill then army-value"),
        (combat_monster_probe.replace("double ratio =",
                                      "volatile double ratio ="),
         "plain T_REAL64 ratio"),
        (combat_monster_probe.replace(
            "int tempNumTroops[7];\n    TCreatureType tempArmies[7];",
            "TCreatureType tempArmies[7];\n    int tempNumTroops[7];"),
         "scoped tempNumTroops then tempArmies"),
    )
    for probe, description in broken_combat_monster_probes:
        if not any(description in rule.description for rule in
                   contract_violations(probe, combat_monster_key)):
            failures.append("broken CombatMonsterEvent " + description
                            + " source shape passed")
    enemy_town_key = ("philai.obj", 0x11105C)
    enemy_town_probe = """\
int creature_cost[NUM_RESOURCES];
unsigned char include_growth;
TCreatureType creature;
hero* defending_hero;
if (population > 0) {
    creature = gTownDwellingCreatures[dwelling];
    GetMonsterCost(creature, creature_cost);
}
"""
    if contract_violations(enemy_town_probe, enemy_town_key):
        failures.append(
            "aligned value_of_enemy_town procedure locals did not pass")
    nested_enemy_town = """\
unsigned char include_growth;
if (population > 0) {
    TCreatureType creature;
    int creature_cost[NUM_RESOURCES];
    GetMonsterCost(creature, creature_cost);
}
"""
    if not contract_violations(nested_enemy_town, enemy_town_key):
        failures.append("nested value_of_enemy_town DC locals passed")
    reordered_enemy_town = enemy_town_probe.replace(
        "int creature_cost[NUM_RESOURCES];\n"
        "unsigned char include_growth;\nTCreatureType creature;",
        "unsigned char include_growth;\nTCreatureType creature;\n"
        "int creature_cost[NUM_RESOURCES];")
    if not contract_violations(reordered_enemy_town, enemy_town_key):
        failures.append("reordered value_of_enemy_town DC locals passed")
    hero_bonuses_key = ("philai.obj", 0x10FEF4)
    hero_bonuses_probe = """\
type_spellvalue caster(our_hero);
our_hero->turnExperienceToRVRatio =
    value_of_experience(our_hero, our_hero->army);
long base_value = caster.get_best_spell_value(SPELL_VALUE_CLASS_MASK);
long value = max(caster.get_value_of_increase(base_value, 1, 1, 0), 10);
our_hero->set_value_of_power(value);
value = caster.get_value_of_increase(base_value, 0, 1, 0);
our_hero->set_value_of_duration(value);
value = max(caster.get_value_of_increase(base_value, 0, 0, 30) / 3, 10);
our_hero->set_value_of_knowledge(value);
if (our_hero->mana >= initial_mana)
    our_hero->set_value_of_well(0);
else
    our_hero->set_value_of_well(caster.get_value_of_increase(
        base_value, 0, 0, initial_mana - our_hero->mana));
if (our_hero->mana >= 2 * initial_mana)
    our_hero->set_value_of_spring(0);
else
    our_hero->set_value_of_spring(caster.get_value_of_increase(
        base_value, 0, 0, 2 * initial_mana - our_hero->mana));
"""
    if contract_violations(hero_bonuses_probe, hero_bonuses_key):
        failures.append("aligned AI_set_hero_bonuses contract did not pass")
    flattened_bonus = hero_bonuses_probe.replace(
        "our_hero->set_value_of_power(value);",
        "our_hero->value_of_power = value;")
    flattened_rules = contract_violations(
        flattened_bonus, hero_bonuses_key)
    if not any("set_value_of_power exactly once" in rule.description
               for rule in flattened_rules):
        failures.append("flattened hero power setter passed")
    if not any("direct field assignments" in rule.description
               for rule in flattened_rules):
        failures.append("direct hero bonus field assignment passed")
    old_maximum = hero_bonuses_probe.replace("max(", "max_ref<long>(")
    if not any("both source-visible max wrappers" in rule.description
               for rule in contract_violations(old_maximum,
                                                hero_bonuses_key)):
        failures.append("AI_set_hero_bonuses max_ref plateau passed")
    renamed_caster = hero_bonuses_probe.replace("caster", "value")
    if not any("sole caster local" in rule.description for rule in
               contract_violations(renamed_caster, hero_bonuses_key)):
        failures.append("renamed AI_set_hero_bonuses caster local passed")
    pointer_bonus = hero_bonuses_probe.replace(
        "value_of_experience(our_hero, our_hero->army)",
        "value_of_experience(our_hero, &our_hero->army)")
    if not any("sole caster local" in rule.description for rule in
               contract_violations(pointer_bonus, hero_bonuses_key)):
        failures.append("pointer-shaped AI_set_hero_bonuses call passed")
    scroll_key = ("philai.obj", 0x112510)
    scroll_probe = """\
SpellID spell;
if (cell->IsCustomized()) {
    TreasureData* treasure = gpAdvManager->get_treasure_data(cell);
    spell = cell->extraInfo & 0xff;
    if (treasure->HasCustomGuardians)
        value += AI_value_of_combat(current_hero, 0,
            treasure->Guardians, 0, cell);
} else
    spell = cell->extraInfo;
type_artifact artifact(spell);
"""
    if contract_violations(scroll_probe, scroll_key):
        failures.append("aligned ValueOfScroll contract did not pass")
    collapsed_scroll = scroll_probe.replace(
        "} else\n    spell = cell->extraInfo;",
        "}\nspell = cell->extraInfo;")
    if not any("assignment arms" in rule.description for rule in
               contract_violations(collapsed_scroll, scroll_key)):
        failures.append("collapsed ValueOfScroll assignment arms passed")
    flattened_scroll = scroll_probe.replace(
        "type_artifact artifact(spell);",
        "type_artifact artifact(ARTIFACT_SPELL_SCROLL);\n"
        "artifact.extra = spell;")
    flattened_scroll_rules = contract_violations(
        flattened_scroll, scroll_key)
    if not any("semantic SpellID constructor" in rule.description
               for rule in flattened_scroll_rules):
        failures.append("flattened ValueOfScroll constructor passed")
    if not any("manual artifact record writes" in rule.description
               for rule in flattened_scroll_rules):
        failures.append("manual ValueOfScroll artifact write passed")
    treasure_key = ("philai.obj", 0x112F6C)
    treasure_probe = """\
int experience_part = 1;
int gold_part = 2;
int value;
if (experience_part > gold_part)
    value = experience_part;
else
    value = gold_part;
"""
    if contract_violations(treasure_probe, treasure_key):
        failures.append("aligned ValueOfTreasure contract did not pass")
    collapsed_treasure = treasure_probe.replace(
        "int value;\nif (experience_part > gold_part)\n"
        "    value = experience_part;\nelse\n"
        "    value = gold_part;",
        "int value = experience_part;\n"
        "if (experience_part <= gold_part)\n"
        "    value = gold_part;")
    if not any("first-pair assignment" in rule.description for rule in
               contract_violations(collapsed_treasure, treasure_key)):
        failures.append("collapsed ValueOfTreasure first pair passed")
    adventure_options_key = ("adventureoptionswindow.obj", 0x5204)
    adventure_options_probe = """\
unsigned char closeDialog = false;
PollSound();
const char* a = gAdventureOptionsHelp[0].text;
const char* b = gAdventureOptionsHelp[1].text;
if (msg->id == MESSAGE_WIDGET) {
    closeDialog = true;
} else if (msg->id == MESSAGE_MOUSE_MOVE) {
    int hoverID = findWidget(msg->mouseX, msg->mouseY);
    if (hoverID) {
        DrawWindow(1, 2, 3);
    }
}
if (closeDialog) {
    msg->id = MESSAGE_WIDGET;
    gpWindowManager->dialogReturn = msg->codeY;
    msg->codeY = widget::WIDGET_END_DIALOG;
    msg->codeX = widget::WIDGET_END_DIALOG;
    return MESSAGE_DISPATCH_FORWARD;
}
return MESSAGE_DISPATCH_CONSUME;
"""
    if contract_violations(adventure_options_probe, adventure_options_key):
        failures.append("aligned adventure-options state contract did not pass")
    split_hover = adventure_options_probe.replace(
        "int hoverID = findWidget(msg->mouseX, msg->mouseY);",
        "int mouseY = msg->mouseY, mouseX = msg->mouseX;\n"
        "    int hoverID = findWidget(mouseX, mouseY);")
    if not any("direct findWidget arguments" in rule.description for rule in
               contract_violations(split_hover, adventure_options_key)):
        failures.append("split adventure-options coordinate locals passed")
    removed_close_state = adventure_options_probe.replace(
        "unsigned char closeDialog = false;\n", "").replace(
            "    closeDialog = true;\n", "").replace(
                "if (closeDialog) {", "if (msg->id == MESSAGE_WIDGET) {")
    if not any("explicit close state" in rule.description for rule in
               contract_violations(removed_close_state,
                                   adventure_options_key)):
        failures.append("adventure-options erased close state passed")
    early_mouse_return = adventure_options_probe.replace(
        "        DrawWindow(1, 2, 3);",
        "        DrawWindow(1, 2, 3);\n"
        "        return MESSAGE_DISPATCH_CONSUME;")
    if not any("one shared consume return" in rule.description for rule in
               contract_violations(early_mouse_return,
                                   adventure_options_key)):
        failures.append("adventure-options early mouse return passed")
    wrong_help_table = adventure_options_probe.replace(
        "gAdventureOptionsHelp", "gAdventureWindowHelp")
    if not any("distinct attested options-help" in rule.description for rule in
               contract_violations(wrong_help_table,
                                   adventure_options_key)):
        failures.append("adventure-options wrong help table passed")
    window_header_probe = """\
int findWidget(int mx, int my) const;
widget* findWidgetPtr(int mx, int my) const;
"""
    if window_header_violations(window_header_probe):
        failures.append("const findWidget header pair did not pass")
    nonconst_window = window_header_probe.replace(" const;", ";")
    if len(window_header_violations(nonconst_window)) != 2:
        failures.append("non-const findWidget header pair passed")
    groups = (CallGroup(10, ("A", "B")), CallGroup(11, ("C",)))
    if misgrouped_from_body("int x = A() + B(); C();", groups):
        failures.append("aligned statement helper groups did not pass")
    if misgrouped_from_body("int x = A(); x += B(); C();", groups):
        failures.append("one breakpoint group could not span adjacent statements")
    broken_order = misgrouped_from_body("B(); A(); C();", groups)
    if broken_order != groups[0]:
        failures.append("reordered helpers inside one group were not detected")
    separate = (CallGroup(20, ("A",)), CallGroup(21, ("B",)))
    if misgrouped_from_body("A() + B();", separate) != separate[1]:
        failures.append("distinct breakpoint groups were flattened together")
    repeated = (CallGroup(20, ("A",)), CallGroup(21, ("A",)))
    if misgrouped_from_body("A();", repeated):
        failures.append("repeated DC-only helper occurrence required equal count")
    nested = (CallGroup(12, ("inner", "outer")),)
    if misgrouped_from_body("return outer(inner());", nested):
        failures.append("nested call evaluation order was treated as lexical")
    controlled = (CallGroup(13, ("Ready",)), CallGroup(14, ("Run",)))
    if misgrouped_from_body("if (Ready()) Run();", controlled):
        failures.append("unbraced control/body statements were conflated")
    do_attack_key = ("army.obj", 0x46BEC)
    contract_probe = """\
army* behind;
gpCombatManager->ResetHitByCreature();
behind = 0;
gpCombatManager->MarkCreatureEffect(get_owning_side(), bitIndex);
gpCombatManager->MarkCreatureEffect(target->get_owning_side(), target->bitIndex);
gpCombatManager->MarkCreatureEffect(behind->get_owning_side(), behind->bitIndex);
if (s->IsValidSeq(1)) {} if (s->IsValidSeq(2)) {}
if (s->IsValidSeq(3)) {}
"""
    if contract_violations(contract_probe, do_attack_key):
        failures.append("aligned no-call/source-order contract did not pass")
    reversed_probe = contract_probe.replace(
        "gpCombatManager->ResetHitByCreature();\nbehind = 0;",
        "behind = 0;\ngpCombatManager->ResetHitByCreature();")
    reversed = contract_violations(reversed_probe, do_attack_key)
    if not any("precedes" in rule.description for rule in reversed):
        failures.append("scheduled declaration order defect was not detected")
    missing_inline = contract_violations(
        contract_probe.replace("s->IsValidSeq(3)", "s->sequenceCount[3]"),
        do_attack_key)
    if not any("IsValidSeq" in rule.description for rule in missing_inline):
        failures.append("flattened no-call accessor was not detected")
    valid_flight_key = ("fly.obj", 0xA1430)
    valid_flight_probe = """\
if (!combatManager::ValidHex(destIndex)) return 0;
if (bLiteralTest || side == -1 || slot == -1) {
    if (get_distance(gridIndex, destIndex) > GetSpeed()) return 0;
    if (!CanFit(destIndex, 0, 0)) return 0;
} else {
    const army* enemy = &gpCombatManager->armies[side][slot];
    if (!find_flyer_attack_cell(enemyHex)) {
        if (!enemy->Is(1u << 0)
                || !find_flyer_attack_cell(
                    enemy->get_second_grid_index()))
            return 0;
    }
}
return 1;
"""
    if contract_violations(valid_flight_probe, valid_flight_key):
        failures.append("aligned ValidFlight source contract did not pass")
    early_success = valid_flight_probe.replace(
        "if (!find_flyer_attack_cell(enemyHex)) {\n"
        "        if (!enemy->Is(1u << 0)\n"
        "                || !find_flyer_attack_cell(\n"
        "                    enemy->get_second_grid_index()))\n"
        "            return 0;\n"
        "    }",
        "if (find_flyer_attack_cell(enemyHex)) return 1;\n"
        "    if (enemy->Is(1u << 0)\n"
        "            && find_flyer_attack_cell(\n"
        "                enemy->get_second_grid_index())) return 1;\n"
        "    return 0;")
    if not any("nested" in rule.description for rule in
               contract_violations(early_success, valid_flight_key)):
        failures.append("ValidFlight positive-return local maximum passed")
    no_literal_else = valid_flight_probe.replace("} else {", "}\n{")
    if not any("line-95 else" in rule.description for rule in
               contract_violations(no_literal_else, valid_flight_key)):
        failures.append("ValidFlight missing enemy-path else passed")
    fly_to_key = ("fly.obj", 0xA1514)
    fly_to_probe = """\
if (combatManager::ValidHex(destIndex)) {
    Fly(destIndex);
    gpCombatManager->TestRaiseDoor();
}
return 0;
"""
    if contract_violations(fly_to_probe, fly_to_key):
        failures.append("aligned FlyTo source contract did not pass")
    flattened_fly_to = fly_to_probe.replace(
        "combatManager::ValidHex(destIndex)",
        "destIndex >= 0 && destIndex < 187").replace(
            "TestRaiseDoor()", "RaiseDoor()")
    flattened_rules = contract_violations(flattened_fly_to, fly_to_key)
    if not any("ValidHex" in rule.description for rule in flattened_rules):
        failures.append("FlyTo manual bounds local maximum passed")
    if not any("TestRaiseDoor" in rule.description
               for rule in flattened_rules):
        failures.append("FlyTo direct RaiseDoor local maximum passed")
    fly_key = ("fly.obj", 0xA1590)
    fly_probe = """\
int sourceX = combatManager::GridX(gridIndex);
int destX = combatManager::GridX(destIndex);
if (Is(1u << 0) && turn)
    destIndex += OffsetToFront(-1);
long iTtlLoops = distance;
long iLoop;
if (!quick) {
    long numFlapFrames = stdIcon->GetNumFrames(cs_walk);
    const int FLY_PERIOD = cycle / numFlapFrames;
    for (iLoop = 0; iLoop < iTtlLoops; iLoop++) {
        for (currFrameIndex = 0; currFrameIndex < numFlapFrames;
                currFrameIndex++) {
            SLimitData TtlExtent = gpCombatManager->drawbridgeBounds;
            gpCombatManager->field_53b0->Draw(
                gpCombatManager->drawbridgeBounds.iMinX,
                gpCombatManager->drawbridgeBounds.iMinY,
                gpCombatManager->drawbridgeBounds.Width(),
                gpCombatManager->drawbridgeBounds.Height());
            bool scrolled = gpCombatManager->ScrollTo(
                gpCombatManager->drawbridgeBounds, true, true, true);
            TtlExtent.Include(gpCombatManager->drawbridgeBounds);
            GameTime::DelayTil(glTimers[0]);
            glTimers[0] = GameTime::NextFrameTime(
                glTimers[0], FLY_PERIOD);
            if (!scrolled)
                gpCombatManager->UpdateCombatArea(TtlExtent);
        }
    }
}
"""
    if contract_violations(fly_probe, fly_key):
        failures.append("aligned Fly source contract did not pass")
    flattened_fly_helpers = fly_probe.replace(
        "combatManager::GridX(gridIndex)", "gridIndex % 17").replace(
            "combatManager::GridX(destIndex)", "destIndex % 17").replace(
                "Is(1u << 0)", "(creatureId & 1)").replace(
                    "OffsetToFront(-1)", "(facing ? 1 : -1)")
    flattened_rules = contract_violations(flattened_fly_helpers, fly_key)
    if not any("GridX" in rule.description for rule in flattened_rules):
        failures.append("Fly manual grid columns local maximum passed")
    if not any("OffsetToFront" in rule.description
               for rule in flattened_rules):
        failures.append("Fly flattened wide-stack arm passed")
    renamed_fly_locals = fly_probe.replace(
        "iTtlLoops", "steps").replace(
            "iLoop", "step").replace(
                "numFlapFrames", "frameCount").replace(
                    "FLY_PERIOD", "frameDelay")
    renamed_rules = contract_violations(renamed_fly_locals, fly_key)
    for local_name in ("iTtlLoops", "iLoop", "numFlapFrames", "FLY_PERIOD"):
        if not any(local_name in rule.description for rule in renamed_rules):
            failures.append(
                f"Fly renamed {local_name} local-minimum spelling passed")
    hoisted_ttl_extent = fly_probe.replace(
        "        for (currFrameIndex = 0; currFrameIndex < numFlapFrames;\n"
        "                currFrameIndex++) {\n"
        "            SLimitData TtlExtent = gpCombatManager->drawbridgeBounds;",
        "        SLimitData TtlExtent = gpCombatManager->drawbridgeBounds;\n"
        "        for (currFrameIndex = 0; currFrameIndex < numFlapFrames;\n"
        "                currFrameIndex++) {")
    if not any("top of the inner frame scope" in rule.description for rule in
               contract_violations(hoisted_ttl_extent, fly_key)):
        failures.append("Fly hoisted TtlExtent lifetime local maximum passed")
    flattened_fly_drawing = fly_probe.replace(
        "gpCombatManager->drawbridgeBounds.Width()",
        "gpCombatManager->drawbridgeBounds.iMaxX - left + 1").replace(
            "gpCombatManager->drawbridgeBounds.Height()",
            "gpCombatManager->drawbridgeBounds.iMaxY - top + 1").replace(
                "bool scrolled = gpCombatManager->ScrollTo(\n"
                "                gpCombatManager->drawbridgeBounds, true, true, true);\n"
                "            TtlExtent.Include(gpCombatManager->drawbridgeBounds);\n"
                "            GameTime::DelayTil(glTimers[0]);\n"
                "            glTimers[0] = GameTime::NextFrameTime(\n"
                "                glTimers[0], FLY_PERIOD);\n"
                "            if (!scrolled)\n"
                "                gpCombatManager->UpdateCombatArea(TtlExtent);",
                "GameTime::DelayTil(glTimers[0]);\n"
                "            gpWindowManager->UpdateScreen(left, top, width, height);")
    flattened_rules = contract_violations(flattened_fly_drawing, fly_key)
    if not any("Width and Height" in rule.description
               for rule in flattened_rules):
        failures.append("Fly flattened Width/Height source facts passed")
    if not any("ScrollTo/Include" in rule.description
               for rule in flattened_rules):
        failures.append("Fly flattened drawing-helper group passed")
    teleport_to_key = ("fly.obj", 0xA19A0)
    teleport_to_probe = fly_to_probe.replace("Fly(destIndex)",
                                             "Teleport(destIndex)")
    if contract_violations(teleport_to_probe, teleport_to_key):
        failures.append("aligned TeleportTo source contract did not pass")
    flattened_teleport_to = teleport_to_probe.replace(
        "combatManager::ValidHex(destIndex)",
        "destIndex >= 0 && destIndex < 187").replace(
            "TestRaiseDoor()", "RaiseDoor()")
    flattened_rules = contract_violations(
        flattened_teleport_to, teleport_to_key)
    if not any("ValidHex" in rule.description for rule in flattened_rules):
        failures.append("TeleportTo manual bounds local maximum passed")
    if not any("TestRaiseDoor" in rule.description
               for rule in flattened_rules):
        failures.append("TeleportTo direct RaiseDoor local maximum passed")
    teleport_key = ("fly.obj", 0xA1A7C)
    teleport_probe = """\
int sourceX = combatManager::GridX(gridIndex);
int destX = combatManager::GridX(destIndex);
if (Is(1u << 0) && turn)
    destIndex += OffsetToFront(-1);
"""
    if contract_violations(teleport_probe, teleport_key):
        failures.append("aligned Teleport source contract did not pass")
    flattened_teleport = teleport_probe.replace(
        "combatManager::GridX(gridIndex)", "gridIndex % 17").replace(
            "combatManager::GridX(destIndex)", "destIndex % 17").replace(
                "Is(1u << 0)", "(creatureId & 1)").replace(
                    "OffsetToFront(-1)", "(facing ? 1 : -1)")
    flattened_rules = contract_violations(flattened_teleport, teleport_key)
    if not any("GridX" in rule.description for rule in flattened_rules):
        failures.append("Teleport manual grid columns local maximum passed")
    if not any("OffsetToFront" in rule.description
               for rule in flattened_rules):
        failures.append("Teleport flattened wide-stack arm passed")
    split_handler_key = ("armygrp.obj", 0x4E428)
    split_handler_probe = """\
int closeDialog = false, updateArmy = false;
int result = CAdvPopup::WindowHandler(msg);
switch (msg->codeX) {
case widget::WIDGET_SELECT:
    switch (msg->codeY) {
    case SPLIT_WIDGET_SOURCE_ENTRY:
        sourceTroops = atoi(msg->extraText);
        break;
    case SPLIT_WIDGET_DESTINATION_ENTRY:
        destinationTroops = atoi(msg->extraText);
        break;
    }
    splitSlider->SetState(destinationTroops);
    updateArmy = true;
    break;
case widget::WIDGET_DESELECT:
    switch (msg->codeY) {
    case DIALOG_RETURN_SPLIT_CLOSE:
    case DIALOG_RETURN_SPLIT_CANCEL:
        gpWindowManager->dialogReturn = msg->codeY;
        break;
    case DIALOG_RETURN_SPLIT_ACCEPT:
        gpWindowManager->dialogReturn = DIALOG_RETURN_SPLIT_ACCEPT;
        break;
    default:
        return MESSAGE_DISPATCH_CONSUME;
    }
    closeDialog = true;
}
break;
case MESSAGE_MOUSE_MOVE:
    if (msg->codeY != gpWindowManager->lastHover) {
        gpWindowManager->lastHover = msg->codeY;
        SetRolloverText(msg->codeY);
    }
    return MESSAGE_DISPATCH_CONSUME;
if (closeDialog == true) {
    msg->codeY = widget::WIDGET_END_DIALOG;
    msg->codeX = widget::WIDGET_END_DIALOG;
    return MESSAGE_DISPATCH_FORWARD;
}
if (updateArmy)
    UpdateSplitArmy(1);
return MESSAGE_DISPATCH_CONSUME;
"""
    if contract_violations(split_handler_probe, split_handler_key):
        failures.append("aligned TSplitWindow handler state contract did not pass")
    removed_handler_state = split_handler_probe.replace(
        "int closeDialog = false, updateArmy = false;\n", "").replace(
            "    updateArmy = true;\n    break;", "    UpdateSplitArmy(1);\n"
            "    return MESSAGE_DISPATCH_CONSUME;").replace(
                "    closeDialog = true;\n", "").replace(
                    "if (updateArmy)\n    UpdateSplitArmy(1);\n", "")
    removed_rules = contract_violations(
        removed_handler_state, split_handler_key)
    if not any("explicit close/update state" in rule.description
               for rule in removed_rules):
        failures.append("TSplitWindow handler erased-state local maximum passed")
    duplicated_handler_tail = split_handler_probe.replace(
        "    msg->codeY = widget::WIDGET_END_DIALOG;\n"
        "    msg->codeX = widget::WIDGET_END_DIALOG;\n"
        "    return MESSAGE_DISPATCH_FORWARD;",
        "    msg->codeY = widget::WIDGET_END_DIALOG;\n"
        "    msg->codeX = widget::WIDGET_END_DIALOG;\n"
        "    return MESSAGE_DISPATCH_FORWARD;\n"
        "    msg->codeY = widget::WIDGET_END_DIALOG;\n"
        "    msg->codeX = widget::WIDGET_END_DIALOG;\n"
        "    return MESSAGE_DISPATCH_FORWARD;")
    duplicated_rules = contract_violations(
        duplicated_handler_tail, split_handler_key)
    if not any("duplicated 99.917% local maximum" in rule.description
               for rule in duplicated_rules):
        failures.append("TSplitWindow handler duplicated-tail local maximum passed")
    duplicated_slider_update = split_handler_probe.replace(
        "        sourceTroops = atoi(msg->extraText);\n        break;",
        "        sourceTroops = atoi(msg->extraText);\n"
        "        splitSlider->SetState(destinationTroops);\n        break;")
    duplicated_slider_rules = contract_violations(
        duplicated_slider_update, split_handler_key)
    if not any("exactly one source-authored slider update" in rule.description
               for rule in duplicated_slider_rules):
        failures.append(
            "TSplitWindow handler duplicated-slider local maximum passed")
    inverted_hover = split_handler_probe.replace(
        "    if (msg->codeY != gpWindowManager->lastHover) {\n"
        "        gpWindowManager->lastHover = msg->codeY;\n"
        "        SetRolloverText(msg->codeY);\n"
        "    }\n    return MESSAGE_DISPATCH_CONSUME;",
        "    if (msg->codeY == gpWindowManager->lastHover)\n"
        "        return MESSAGE_DISPATCH_CONSUME;\n"
        "    gpWindowManager->lastHover = msg->codeY;\n"
        "    SetRolloverText(msg->codeY);\n    break;")
    inverted_hover_rules = contract_violations(
        inverted_hover, split_handler_key)
    if not any("positive hover-change scope" in rule.description
               for rule in inverted_hover_rules):
        failures.append(
            "TSplitWindow handler inverted-hover local maximum passed")
    get_morale_key = ("armygrp.obj", 0x4F078)
    get_morale_probe = """\
morale += 2 - numAlignments;
if (HasSomeUndead())
    morale--;
if (IsMember(CREATURE_ANGEL) || IsMember(CREATURE_ARCHANGEL))
    morale++;
"""
    if contract_violations(get_morale_probe, get_morale_key):
        failures.append("aligned GetMorale undead-helper boundary did not pass")
    flattened_get_morale = get_morale_probe.replace(
        "if (HasSomeUndead())\n    morale--;",
        "for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {\n"
        "    if (akCreatureTypeTraits[armies[i]].attributes & CTA_UNDEAD)\n"
        "        morale--;\n}")
    if not any("HasSomeUndead member boundary" in rule.description
               for rule in contract_violations(flattened_get_morale,
                                                get_morale_key)):
        failures.append("flattened GetMorale undead scan passed")
    morale_description_key = ("armygrp.obj", 0x4F708)
    morale_description_probe = """\
if (HasSomeUndead())
    result.append(gUndeadMoraleText);
TCreatureType angelType = CREATURE_NONE;
if (IsMember(CREATURE_ANGEL))
    angelType = CREATURE_ANGEL;
"""
    if contract_violations(morale_description_probe,
                           morale_description_key):
        failures.append(
            "aligned morale-description undead-helper boundary did not pass")
    flattened_morale_description = morale_description_probe.replace(
        "if (HasSomeUndead())",
        "if (armygrp_has_some_undead(this))")
    if not any("HasSomeUndead member boundary" in rule.description
               for rule in contract_violations(
                   flattened_morale_description, morale_description_key)):
        failures.append("flattened morale-description undead scan passed")
    split_ctor_key = ("armygrp.obj", 0x4DBB8)
    split_ctor_probe = """\
sprintf(gText, (*gpGeneralText)[GENERAL_TEXT_SPLIT_CREATURE_ROLLOVER], name);
sourceEntry = new textEntryWidget(1);
Widgets.push_back(sourceEntry);
destinationEntry = new textEntryWidget(2);
Widgets.push_back(destinationEntry);
splitSlider = new slider(3, slider::BROWN);
Widgets.push_back(splitSlider);
Widgets.push_back(p); Widgets.push_back(p); Widgets.push_back(p);
Widgets.push_back(p); Widgets.push_back(p); Widgets.push_back(p);
Widgets.push_back(p);
Widgets.push_back(new textWidget(8, 312, 282, 17, 0, "smalfont.fnt", 1));
Widgets.push_back(new button(DIALOG_RETURN_SPLIT_ACCEPT, 0));
Widgets.push_back(new button(0x7801, 0));
"""
    if contract_violations(split_ctor_probe, split_ctor_key):
        failures.append("aligned TSplitWindow constructor contract did not pass")
    twelve_pushes = split_ctor_probe.replace(
        "Widgets.push_back(p); Widgets.push_back(p); Widgets.push_back(p);",
        "Widgets.push_back(p); Widgets.push_back(p);", 1)
    fourteen_pushes = split_ctor_probe.replace(
        "Widgets.push_back(p); Widgets.push_back(p); Widgets.push_back(p);",
        "Widgets.push_back(p); Widgets.push_back(p); Widgets.push_back(p); "
        "Widgets.push_back(p);", 1)
    count_description = "exactly thirteen"
    if not any(count_description in rule.description for rule in
               contract_violations(twelve_pushes, split_ctor_key)):
        failures.append("TSplitWindow missing push_back passed exact-count gate")
    if not any(count_description in rule.description for rule in
               contract_violations(fourteen_pushes, split_ctor_key)):
        failures.append("TSplitWindow extra push_back passed exact-count gate")
    insert_local_maximum = split_ctor_probe.replace(
        "Widgets.push_back(new button(DIALOG_RETURN_SPLIT_ACCEPT, 0));\n"
        "Widgets.push_back(new button(0x7801, 0));",
        "widgetList->insert(widgetList->end(), "
        "new button(DIALOG_RETURN_SPLIT_ACCEPT, 0));\n"
        "AppendSplitWidget(*widgetList, widgetList->end(), "
        "new button(0x7801, 0));")
    if not contract_violations(insert_local_maximum, split_ctor_key):
        failures.append("TSplitWindow insert/adapter local maximum passed")
    empty_status_text = split_ctor_probe.replace(
        '17, 0, "smalfont.fnt"', '17, "", "smalfont.fnt"')
    if not any("null text" in rule.description for rule in
               contract_violations(empty_status_text, split_ctor_key)):
        failures.append("TSplitWindow empty-string status text passed null gate")
    special_terrain_key = ("hero.obj", 0xD4DF0)
    special_terrain_probe = """\
type_point location = get_location();
if (location == type_point(-1, -1, -1))
    return kMagicTerrainNone;
NewmapCell* cell = gpGame->get_cell(location);
return cell->get_special_terrain();
"""
    if contract_violations(special_terrain_probe, special_terrain_key):
        failures.append("aligned get_special_terrain contract did not pass")
    flattened_special_terrain = special_terrain_probe.replace(
        "type_point location = get_location();",
        "type_point location; location.x = x; location.y = y; "
        "location.z = z;").replace(
            "if (location == type_point(-1, -1, -1))",
            "if (location.x == -1 && location.y == -1 && location.z == -1)")
    flattened_rules = contract_violations(
        flattened_special_terrain, special_terrain_key)
    if not any("get_location local" in rule.description
               for rule in flattened_rules):
        failures.append("get_special_terrain flattened get_location passed")
    if not any("operator==" in rule.description for rule in flattened_rules):
        failures.append("get_special_terrain flattened sentinel test passed")
    chained_special_terrain = special_terrain_probe.replace(
        "NewmapCell* cell = gpGame->get_cell(location);\n"
        "return cell->get_special_terrain();",
        "return gpGame->get_cell(location)->get_special_terrain();")
    if not any("separate Dreamcast statements" in rule.description
               for rule in contract_violations(
                   chained_special_terrain, special_terrain_key)):
        failures.append("get_special_terrain chained cell access passed")
    mana_cost_key = ("hero.obj", 0xD4F64)
    mana_cost_probe = """\
if (iWhichSpell == SPELL_ARMAGEDDON
        && IsWieldingArtifact(ARTIFACT_ARMAGEDDONS_BLADE))
    mastery = eMasteryExpert;
else
    mastery = GetSpellSchoolLevel(
        akSpellTraits[iWhichSpell].school, magic_terrain);
if (HasArmy(CREATURE_MAGE) || HasArmy(CREATURE_ARCH_MAGE))
    cost -= 2;
"""
    if contract_violations(mana_cost_probe, mana_cost_key):
        failures.append("aligned GetManaCost source contract did not pass")
    flattened_mana_cost = mana_cost_probe.replace(
        "GetSpellSchoolLevel(\n"
        "        akSpellTraits[iWhichSpell].school, magic_terrain)",
        "get_spell_level(iWhichSpell, magic_terrain)").replace(
            "HasArmy(CREATURE_MAGE)",
            "army.IsMember(CREATURE_MAGE)").replace(
                "HasArmy(CREATURE_ARCH_MAGE)",
                "army.IsMember(CREATURE_ARCH_MAGE)")
    flattened_rules = contract_violations(flattened_mana_cost, mana_cost_key)
    if not any("GetSpellSchoolLevel" in rule.description
               for rule in flattened_rules):
        failures.append("GetManaCost flattened school-level helper passed")
    if not any("HasArmy" in rule.description for rule in flattened_rules):
        failures.append("GetManaCost flattened HasArmy wrappers passed")
    hero_fly_key = ("hero.obj", 0xD5488)
    hero_fly_probe = """\
flightLevel = level;
UseSpell(GetManaCost(SPELL_FLY));
"""
    if contract_violations(hero_fly_probe, hero_fly_key):
        failures.append("aligned hero::Fly source contract did not pass")
    flattened_hero_fly = """\
flightLevel = level;
int cost = GetManaCost(SPELL_FLY);
UseSpell(cost);
"""
    if not contract_violations(flattened_hero_fly, hero_fly_key):
        failures.append("hero::Fly unnested mana-cost local maximum passed")
    type_point_probe = """\
bool operator==(const type_point& arg) const {
    return x == arg.x && y == arg.y && z == arg.z;
}
"""
    if type_point_header_violations(type_point_probe):
        failures.append("aligned type_point equality operator did not pass")
    bad_type_point_probes = (
        type_point_probe.replace("const type_point& arg", "type_point* arg"),
        type_point_probe.replace(") const {", ") {"),
        type_point_probe.replace(
            "x == arg.x && y == arg.y && z == arg.z",
            "x == arg.x && z == arg.z && y == arg.y"),
    )
    if any(not type_point_header_violations(probe)
           for probe in bad_type_point_probes):
        failures.append("broken type_point equality source shape passed")
    get_location_probe = """\
    type_point get_location() const
    {
        return type_point(x, y, z);
    }
"""
    if hero_get_location_header_violations(get_location_probe):
        failures.append("aligned Hero.h get_location shape did not pass")
    broken_get_location_probes = (
        get_location_probe.replace(
            "    type_point get_location() const",
            "    __forceinline type_point get_location() const"),
        get_location_probe.replace("x, y, z", "x, z, y"),
        get_location_probe.replace("() const", "()"),
    )
    if any(not hero_get_location_header_violations(probe)
           for probe in broken_get_location_probes):
        failures.append("broken Hero.h get_location source shape passed")
    get_target_probe = """\
__forceinline type_point get_target() const
{
    return type_point(pathTargetX, pathTargetY, pathTargetZ);
}
"""
    if hero_get_target_header_violations(get_target_probe):
        failures.append("aligned Hero.h get_target source shape did not pass")
    broken_get_target = get_target_probe.replace(
        "pathTargetX, pathTargetY, pathTargetZ",
        "pathTargetX, pathTargetZ, pathTargetY")
    if not hero_get_target_header_violations(broken_get_target):
        failures.append("broken Hero.h get_target field order passed")
    hide_hero_probe = """\
enum eRS_Messages {
    RS_GAME_HEADER_INFO = 1023,
    RS_GAME_HEADER_INFO_INIT = 1024,
    RS_GAME_HEADER_INFO_END = 1025,
    RS_NEW_SETUP_INFO = 1026,
    RS_SCROLL = 1027,
    RS_NEW_MAP_HEADER_INFO = 1028,
    RS_MAP_HEADER_REQUEST = 1029,
    RS_MAP_FILE_NAME = 1030,
    RS_SORT_MAPS = 1031,
    RS_SET_FILTER = 1032,
    RS_REQUEST_HERO_FACE = 1035,
    RS_REQUEST_HERO_FACE_REPLY = 1036,
    RS_SETAGR = 1037,
    RS_NEW_HOST = 1038,
    RS_UPDATE_PLAYER_POS = 1039,
    RS_NEW_PLAYER = 1040,
    RS_REQ_HEADER_CONFIRM = 1041,
    RS_HEADER_CONFIRM = 1042,
    RS_CLICK = 1043,
    RS_TOWN_UPDATE = 1044,
    RS_LAUNCHING_GAME = 1045,
    RS_BAD_VERSION = 1046,
    RS_GAME_TRANSMIT_PENDING = 1082,
    RS_GAME_HEADER_INFO_INIT_EX = 1083,
    RS_HEADERS_REQUEST = 1084,
};
class CNetMsg {
public:
    CNetMsg(eRS_Messages subType, unsigned long size)
    {
        this->subType = subType;
        field_00 = -1;
        this->size = size;
        field_04 = 0;
        field_10 = 0;
    }
};
class CMapChange : public CNetMsg {
public:
    CMapChange(eRS_Messages id, unsigned long size)
        : CNetMsg(id, size) {}
};
class CMCHideHero : public CMapChange {
public:
    int heroId;
    CMCHideHero(int heroId)
        : CMapChange(RS_HIDE_HERO, sizeof(CMCHideHero))
    {
        this->heroId = heroId;
    }
};
"""
    if cmc_hide_hero_header_violations(hide_hero_probe):
        failures.append("aligned CNetMsg constructor chain did not pass")
    broken_hide_hero_probes = (
        (hide_hero_probe.replace("RS_SCROLL = 1027", "RS_SCROLL = 2027"),
         "netmsg.h eRS_Messages"),
        (hide_hero_probe.replace(
            "CNetMsg(eRS_Messages subType, unsigned long size)",
            "CNetMsg(int new_sub_type, unsigned long new_size)").replace(
                "this->subType = subType", "subType = new_sub_type").replace(
                    "this->size = size", "size = new_size"),
         "netmsg.h:167 CNetMsg"),
        (hide_hero_probe.replace(
            "CMapChange(eRS_Messages id, unsigned long size)",
            "CMapChange(eRS_Messages id, unsigned long messageSize)").replace(
                "CNetMsg(id, size)", "CNetMsg(id, messageSize)"),
         "netmsg.h:532 CMapChange"),
        (hide_hero_probe.replace(
            "CMCHideHero(int heroId)", "CMCHideHero(int id)").replace(
                "this->heroId = heroId", "heroId = id"),
         "netmsg.h:717 CMCHideHero"),
        (hide_hero_probe.replace(
            ": CMapChange(RS_HIDE_HERO, sizeof(CMCHideHero))",
            ": CMapChange()"),
         "netmsg.h:717 CMCHideHero"),
    )
    for probe, description in broken_hide_hero_probes:
        if not any(description in defect for _line, defect in
                   cmc_hide_hero_header_violations(probe)):
            failures.append("broken " + description
                            + " constructor shape passed")
    claim_message_probe = """\
class CMCClaimMine : public CMapChange {
public:
    signed char mineId;
    int playerPos;
    CMCClaimMine(signed char id, int player)
        : CMapChange(RS_CLAIM_MINE, sizeof(CMCClaimMine))
    {
        mineId = id;
        playerPos = player;
    }
};
class CMCClaimTown : public CMapChange {
public:
    signed char townId;
    int playerPos;
    CMCClaimTown(signed char id, int player)
        : CMapChange(RS_CLAIM_TOWN, sizeof(CMCClaimTown))
    {
        townId = id;
        playerPos = player;
    }
};
"""
    if cmc_claim_header_violations(claim_message_probe):
        failures.append("aligned claim-message constructors did not pass")
    broken_claim_message_probes = (
        claim_message_probe.replace(
            "    {\n        mineId = id;\n        playerPos = player;\n    }",
            ", mineId(id), playerPos(player) {}"),
        claim_message_probe.replace(
            "        townId = id;\n        playerPos = player;",
            "        playerPos = player;\n        townId = id;"),
        claim_message_probe.replace(
            "CMapChange(RS_CLAIM_TOWN, sizeof(CMCClaimTown))",
            "CMapChange()"),
    )
    if any(not cmc_claim_header_violations(probe)
           for probe in broken_claim_message_probes):
        failures.append("broken claim-message constructor shape passed")
    get_hero_probe = """\
hero* GetHero(int which)
{
    if (which == -1)
        return 0;
    return heroes + which;
}
"""
    if game_get_hero_header_violations(get_hero_probe):
        failures.append("aligned Game.h GetHero source shape did not pass")
    indexed_get_hero = get_hero_probe.replace(
        "return heroes + which;", "return &heroes[which];")
    if game_get_hero_header_violations(indexed_get_hero):
        failures.append("equivalent indexed Game.h GetHero shape did not pass")
    broken_get_hero_probes = (
        get_hero_probe.replace("int which", "int heroId").replace(
            "which", "heroId"),
        get_hero_probe.replace(
            "    if (which == -1)\n        return 0;\n", ""),
        get_hero_probe.replace(
            "    return heroes + which;",
            "    hero* result = heroes + which;\n    return result;"),
    )
    if any(not game_get_hero_header_violations(probe)
           for probe in broken_get_hero_probes):
        failures.append("broken Game.h GetHero source shape passed")
    computer_team_probe = """\
inline unsigned char IsComputerTeam(int teamNum) const
{
    if (teamNum < 0)
        return 0;
    return !is_human_ally(teamNum);
}
"""
    if game_is_computer_team_header_violations(computer_team_probe):
        failures.append("aligned Game.h IsComputerTeam shape did not pass")
    broken_computer_team_probes = (
        computer_team_probe.replace("unsigned char", "bool"),
        computer_team_probe.replace(
            "    if (teamNum < 0)\n        return 0;\n", ""),
        computer_team_probe.replace("is_human_ally", "IsHumanTeam"),
    )
    if any(not game_is_computer_team_header_violations(probe)
           for probe in broken_computer_team_probes):
        failures.append("broken Game.h IsComputerTeam shape passed")
    randomize_header_probe = """\
void match_underground_gates();
void CompleteOnlyHelper();
void randomize_university(NewmapCell* cell);
"""
    if game_randomize_header_violations(randomize_header_probe):
        failures.append("aligned game randomize member order did not pass")
    broken_randomize_headers = (
        randomize_header_probe.replace(
            "void match_underground_gates();\n"
            "void CompleteOnlyHelper();\n"
            "void randomize_university(NewmapCell* cell);",
            "void randomize_university(NewmapCell* cell);\n"
            "void CompleteOnlyHelper();\n"
            "void match_underground_gates();"),
        randomize_header_probe.replace("NewmapCell* cell", "void* cell"),
    )
    if any(not game_randomize_header_violations(probe)
           for probe in broken_randomize_headers):
        failures.append("broken game randomize member order passed")
    update_msg_probe = """\
class CUpdatePlayerPosMsg : public CNetMsg {
    CNetPlayerHandlerPlayer m_netPlayer[8];
    CNetPlayerHandlerPlayer m_compPlayer[8];
    CUpdatePlayerPosMsg(CNetPlayerHandlerPlayer* pNetPlayers,
                        CNetPlayerHandlerPlayer* pCompPlayers);
};
"""
    update_msg_source_probe = """\
inline CUpdatePlayerPosMsg::CUpdatePlayerPosMsg(
    CNetPlayerHandlerPlayer* pNetPlayers,
    CNetPlayerHandlerPlayer* pCompPlayers)
    : CNetMsg(RS_UPDATE_PLAYER_POS, sizeof(CUpdatePlayerPosMsg))
{
    memcpy(m_netPlayer, pNetPlayers, sizeof(m_netPlayer));
    memcpy(m_compPlayer, pCompPlayers, sizeof(m_compPlayer));
}
"""
    if single_selection_window_contract_violations(
            update_msg_probe, update_msg_source_probe):
        failures.append("aligned CUpdatePlayerPosMsg constructor did not pass")
    broken_update_probes = (
        (update_msg_probe.replace("m_netPlayer[8]", "m_players[8]"),
         update_msg_source_probe),
        (update_msg_probe, update_msg_source_probe.replace(
            ": CNetMsg(RS_UPDATE_PLAYER_POS, sizeof(CUpdatePlayerPosMsg))",
            "")),
        (update_msg_probe, update_msg_source_probe.replace(
            "memcpy(m_netPlayer, pNetPlayers, sizeof(m_netPlayer));\n"
            "    memcpy(m_compPlayer, pCompPlayers, "
            "sizeof(m_compPlayer));",
            "memcpy(m_compPlayer, pCompPlayers, sizeof(m_compPlayer));\n"
            "    memcpy(m_netPlayer, pNetPlayers, "
            "sizeof(m_netPlayer));")),
    )
    if any(not single_selection_window_contract_violations(header, source)
           for header, source in broken_update_probes):
        failures.append("broken CUpdatePlayerPosMsg constructor shape passed")
    netplayer_header_probe = """\
class CNetPlayerInfo {
public:
    unsigned long dpid;
    char sName[24];
    int version;
    CNetPlayerInfo();
};
"""
    netplayer_source_probe = """\
inline CNetPlayerInfo::CNetPlayerInfo()
{
    dpid = 0;
    sName[0] = 0;
    version = *gpVideoGameState;
}
CNetPlayerHandlerPlayer::CNetPlayerHandlerPlayer()
{
    heroIndex = -1;
    townIndex = -1;
}
"""
    if netplayer_constructor_contract_violations(
            netplayer_header_probe, netplayer_source_probe):
        failures.append("aligned CNetPlayerInfo constructor did not pass")
    broken_netplayer_probes = (
        (netplayer_header_probe.replace("CNetPlayerInfo();", ""),
         netplayer_source_probe),
        (netplayer_header_probe, netplayer_source_probe.replace(
            "dpid = 0;\n    sName[0] = 0;",
            "sName[0] = 0;\n    dpid = 0;")),
        (netplayer_header_probe, netplayer_source_probe.replace(
            "version = *gpVideoGameState;\n}",
            "}\n").replace(
                "{\n    heroIndex = -1;",
                "{\n    version = *gpVideoGameState;\n    heroIndex = -1;")),
    )
    if any(not netplayer_constructor_contract_violations(header, source)
           for header, source in broken_netplayer_probes):
        failures.append("broken CNetPlayerInfo constructor shape passed")
    update_pos_header_probe = """\
void OnUpdatePlayerPosMsg(CNetMsg* pNetMsg);
"""
    update_pos_source_probe = """\
VA(0x0058BA40, 0x175)
void TSingleSelectionWindow::OnUpdatePlayerPosMsg(CNetMsg* pNetMsg)
{
}
"""
    if update_player_pos_signature_violations(
            update_pos_header_probe, update_pos_source_probe):
        failures.append("aligned OnUpdatePlayerPosMsg signature did not pass")
    broken_update_pos_probes = (
        (update_pos_header_probe.replace("void ", "unsigned char "),
         update_pos_source_probe),
        (update_pos_header_probe,
         update_pos_source_probe.replace(
             "void TSingleSelectionWindow",
             "unsigned char TSingleSelectionWindow")),
        (update_pos_header_probe,
         update_pos_source_probe.replace("0x0058BA40", "0x0058BA44")),
    )
    if any(not update_player_pos_signature_violations(header, source)
           for header, source in broken_update_pos_probes):
        failures.append("broken OnUpdatePlayerPosMsg signature passed")
    split_header_probe = """\
TCreatureType creature;
slider* splitSlider;
TSplitWindow(int x2, int y2, TCreatureType thisArmy);
"""
    if split_window_header_violations(split_header_probe):
        failures.append("aligned TSplitWindow domain types did not pass")
    if not split_window_header_violations(
            split_header_probe.replace("TCreatureType creature", "int creature")):
        failures.append("int TSplitWindow creature field passed")
    if not split_window_header_violations(
            split_header_probe.replace(
                "TCreatureType thisArmy", "int thisArmy")):
        failures.append("int TSplitWindow constructor parameter passed")
    undead_header_probe = "unsigned char HasSomeUndead() const;\n"
    undead_source_probe = """\
unsigned char armyGroup::HasSomeUndead() const
{
    for (int i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (armies[i] == CREATURE_NONE)
            continue;
        if (akCreatureTypeTraits[armies[i]].attributes & CTA_UNDEAD)
            return 1;
    }
    return 0;
}
"""
    if armygroup_has_some_undead_violations(
            undead_header_probe, undead_source_probe):
        failures.append("aligned HasSomeUndead member body did not pass")
    broken_undead_probes = (
        (undead_header_probe.replace(" const", ""), undead_source_probe),
        (undead_header_probe,
         undead_source_probe.replace("if (armies[i] == CREATURE_NONE)",
                                     "if (armies[i] != CREATURE_NONE)")),
    )
    if any(not armygroup_has_some_undead_violations(header, source)
           for header, source in broken_undead_probes):
        failures.append("broken HasSomeUndead member boundary passed")
    const_query_names = (
        "HasCreatures", "HasAllUndead", "HasSomeUndead", "IsMember",
        "CanJoin", "GetAlignments", "get_AI_value", "GetNativeTerrain",
        "GetNumArmies", "GetMorale", "GetArmyMorale", "GetLuck",
        "GetArmyLuck", "get_morale_description", "get_luck_description",
    )
    const_query_header_probe = "\n".join(
        f"int {name}(int value) const;" for name in const_query_names)
    const_query_header_probe += (
        "\nint get_creature_total() const;\n"
        "int get_creature_total(TCreatureType type) const;\n")
    const_query_source_probe = "\n".join(
        f"int armyGroup::{name}(int value) const {{ return value; }}"
        for name in const_query_names)
    const_query_source_probe += (
        "\nint armyGroup::get_creature_total() const { return 0; }\n"
        "int armyGroup::get_creature_total(TCreatureType type) const "
        "{ return type; }\n"
        "int armyGroup::GetHomogeneityMoraleAdjust() const { return 0; }\n")
    if armygroup_const_query_violations(
            const_query_header_probe, const_query_source_probe):
        failures.append("aligned armyGroup QB const-query surface did not pass")
    nonconst_query_header = const_query_header_probe.replace(
        "int HasCreatures(int value) const;",
        "int HasCreatures(int value);")
    if not armygroup_const_query_violations(
            nonconst_query_header, const_query_source_probe):
        failures.append("non-const armyGroup query declaration passed")
    nonconst_query_source = const_query_source_probe.replace(
        "int armyGroup::get_AI_value(int value) const",
        "int armyGroup::get_AI_value(int value)")
    if not armygroup_const_query_violations(
            const_query_header_probe, nonconst_query_source):
        failures.append("non-const armyGroup query definition passed")
    adapter_query_header = const_query_header_probe.replace(
        "int get_AI_value(int value) const;",
        "int get_AI_value(int value);\n"
        "int get_AI_value(int value) const { return "
        "const_cast<armyGroup*>(this)->get_AI_value(value); }")
    if not armygroup_const_query_violations(
            adapter_query_header, const_query_source_probe):
        failures.append("armyGroup non-const query plus const adapter passed")
    cmbtmgr_inline_probe = """\
static bool ValidHex(int iHex) {
    return iHex >= 0 && iHex < COMBAT_GRID_CELLS;
}
army* get_current_army() { return &armies[actingSide][actingSlot]; }
static int GetHexIndex(int x, int y) {
    return y * COMBAT_GRID_ROW_STRIDE + x;
}
bool RowIsOdd(int y) const { return (y & 1) != 0; }
static int GridX(int index) { return index % COMBAT_GRID_ROW_STRIDE; }
static bool InInvisibleColumn(int index) {
    if (!ValidHex(index)) return false;
    int column = GridX(index);
    return column == 0 || column == COMBAT_GRID_LAST_COLUMN;
}
hexcell& GetCell(int x, int y) { return cells[GetHexIndex(x, y)]; }
TObstacle& GetObstacle(int index) { return obstacles.begin[index]; }
void TestRaiseDoor() { RaiseDoor(); }
"""
    if combat_manager_header_violations(cmbtmgr_inline_probe):
        failures.append("aligned cmbtmgr.h inline band did not pass")
    if not combat_manager_header_violations(
            cmbtmgr_inline_probe.replace("static bool ValidHex",
                                         "unsigned char ValidHex")):
        failures.append("non-static/non-bool ValidHex passed")
    if not combat_manager_header_violations(
            cmbtmgr_inline_probe.replace("static int GridX",
                                         "int GridX")):
        failures.append("non-static GridX passed")
    reordered_cmbtmgr = cmbtmgr_inline_probe.replace(
        "static int GetHexIndex(int x, int y) {\n"
        "    return y * COMBAT_GRID_ROW_STRIDE + x;\n"
        "}\n"
        "bool RowIsOdd(int y) const { return (y & 1) != 0; }",
        "bool RowIsOdd(int y) const { return (y & 1) != 0; }\n"
        "static int GetHexIndex(int x, int y) {\n"
        "    return y * COMBAT_GRID_ROW_STRIDE + x;\n"
        "}")
    if not any("inline order" in defect for _line, defect in
               combat_manager_header_violations(reordered_cmbtmgr)):
        failures.append("reordered cmbtmgr.h inline band passed")
    is_header = """\
bool Is(unsigned attribute) const {
  return (sMonInfo.creatureId & attribute) != 0;
  return (creatureId & attribute) != 0;
}
int get_owning_side() const { return combatSide; }
"""
    if army_is_contract_violations(is_header, header=True):
        failures.append("mask-based army::Is header did not pass")
    bad_is = is_header.replace("creatureId & attribute",
                               "creatureId >> attribute") + "Is(19);\n"
    bad_defects = army_is_contract_violations(bad_is, header=True)
    if not any("header arms" in defect for _line, defect in bad_defects):
        failures.append("shift-based army::Is body was not detected")
    if not any("bit index" in defect for _line, defect in bad_defects):
        failures.append("bit-index army::Is call was not detected")
    guarded = "#ifdef HOMM3_ARMY_AURA_VIEW\n" + is_header + "#endif\n"
    if not any("TU view" in defect for _line, defect in
               army_is_contract_violations(guarded, header=True)):
        failures.append("view-hidden get_owning_side was not detected")

    roster_text = (common.HOMM3_DIR / "include/army.h").read_text()
    if roster_defects := army_header_roster_violations(roster_text):
        failures.append("current Army.h roster does not pass its own gate: "
                        + roster_defects[0][1])
    missing_roster = roster_text.replace(
        "inline bool army::IsActive() const",
        "inline bool army::IsDormant() const", 1)
    if not any("IsActive" in defect for _line, defect in
               army_header_roster_violations(missing_roster)):
        failures.append("removed Army.h roster member was not detected")
    roster_spans, _ = _army_roster_spans(_source.mask(roster_text))
    span_by_line = {line: (start, end + 1)
                    for line, start, end, _name in roster_spans}
    first_start, first_end = span_by_line[770]
    second_start, second_end = span_by_line[775]
    swapped_roster = (roster_text[:first_start]
                      + roster_text[second_start:second_end]
                      + roster_text[first_end:second_start]
                      + roster_text[first_start:first_end]
                      + roster_text[second_end:])
    if not any("out of order" in defect for _line, defect in
               army_header_roster_violations(swapped_roster)):
        failures.append("reordered Army.h roster members were not detected")
    active_start, active_end = span_by_line[830]
    guarded_roster = (roster_text[:active_start] + "#ifdef BROKEN_VIEW\n"
                      + roster_text[active_start:active_end]
                      + "\n#endif\n" + roster_text[active_end:])
    if not any("TU view" in defect for _line, defect in
               army_header_roster_violations(guarded_roster)):
        failures.append("view-hidden Army.h roster member was not detected")

    class_defects = {description for _line, description in
                     army_class_roster_violations(roster_text)}
    swapped_prefix = roster_text.replace(
        "    int Fly(int destIndex);\n"
        "    int FlyTo(int destIndex, unsigned char restore_facing);",
        "    int FlyTo(int destIndex, unsigned char restore_facing);\n"
        "    int Fly(int destIndex);", 1)
    swapped_defects = {description for _line, description in
                       army_class_roster_violations(swapped_prefix)}
    if not any("army::FlyTo after army::Fly" in defect
               for defect in swapped_defects - class_defects):
        failures.append("swapped LF_FIELDLIST method declarations passed")
    explicit_dtor = roster_text.replace(
        "    bool LeavesNoBody() const;",
        "    ~army();\n    bool LeavesNoBody() const;", 1)
    if not any("compgen" in defect for _line, defect in
               army_class_roster_violations(explicit_dtor)):
        failures.append("explicit source destructor passed compgen gate")
    wrong_bool = roster_text.replace(
        "    bool IsActive() const;",
        "    unsigned char IsActive() const;", 1)
    if not any("IsActive returns bool" in defect for _line, defect in
               army_class_roster_violations(wrong_bool)):
        failures.append("non-bool Army predicate declaration passed")
    public_leaf = roster_text.replace(
        "private:\n    void animate_missile(army* armyToAttack);",
        "public:\n    void animate_missile(army* armyToAttack);", 1)
    if not any("LeavesNoBody is private" in defect for _line, defect in
               army_class_roster_violations(public_leaf)):
        failures.append("private LF_FIELDLIST method passed as public")
    return failures


def _xref_calls() -> dict[tuple[str, int], list[XrefCall]]:
    if not XREFS.is_file():
        common.die(f"missing Dreamcast xref graph: {XREFS}")
    data = "\n".join(line for line in XREFS.read_text().splitlines()
                     if not line.startswith("#"))
    out: dict[tuple[str, int], list[XrefCall]] = {}
    for row in csv.DictReader(io.StringIO(data), delimiter="\t"):
        key = row["src_module"], int(row["src_offset"], 0)
        out.setdefault(key, []).append(XrefCall(
            int(row["dst_offset"], 0), row["dst_name"],
            int(row["pool_refs"]), int(row["bsr_calls"])))
    return out


def _attested_names(refs: list[XrefCall], decoded_pool_calls: set[int]) \
        -> set[str]:
    """Names proved to be calls, excluding literal-pool data collisions."""
    return {ref.name for ref in refs
            if ref.bsr_calls or ref.offset in decoded_pool_calls}


def _all_line_rows(dump: list[str]) -> list[tuple[int, int, str]]:
    """All CodeView ``(address, line, source)`` rows, indexed once."""
    rows: list[tuple[int, int, str]] = []
    in_table = False
    source = ""
    for line in dump:
        if match := dc_lines.LINETAB_RE.match(line):
            in_table = True
            source = match.group(1)
            continue
        if in_table and "***" in line:
            in_table = False
            continue
        if in_table:
            rows.extend((int(match.group(2), 16), int(match.group(1)), source)
                        for match in dc_lines.PAIR_RE.finditer(line))
    return sorted(rows)


def _decode_sh4_control(data: bytes):
    """Fast fixed-width decoder for the control opcodes CFG recovery needs.

    Full Capstone text decoding is useful in ``homm3 dreamcast asm`` but made
    this fatal whole-corpus build gate take minutes.  Every SH4 instruction is
    two bytes; recognizing only branches, jumps and returns gives build_cfg
    the identical reachability decisions without formatting every arithmetic
    instruction.
    """
    names = {0x8900: "bt", 0x8B00: "bf", 0x8D00: "bt/s", 0x8F00: "bf/s"}

    def decode(address: int) -> dc_asm.Instruction:
        raw = data[dc_lines.TEXT_RAW + address:
                   dc_lines.TEXT_RAW + address + 2]
        if len(raw) != 2:
            raise dc_asm.AsmError(f"truncated SH4 instruction at dc {address:#x}")
        word = int.from_bytes(raw, "little")
        mnemonic, operands = ".word", f"0x{word:04x}"
        high = word & 0xFF00
        if high in names:
            disp = word & 0xFF
            if disp & 0x80:
                disp -= 0x100
            mnemonic, operands = names[high], hex(address + 4 + disp * 2)
        elif (word & 0xF000) in {0xA000, 0xB000}:
            disp = word & 0xFFF
            if disp & 0x800:
                disp -= 0x1000
            mnemonic = "bra" if (word & 0xF000) == 0xA000 else "bsr"
            operands = hex(address + 4 + disp * 2)
        elif word == 0x000B:
            mnemonic, operands = "rts", ""
        elif word == 0x002B:
            mnemonic, operands = "rte", ""
        elif (word & 0xF0FF) == 0x402B:
            mnemonic, operands = "jmp", f"@r{(word >> 8) & 0xF}"
        elif (word & 0xF0FF) == 0x0023:
            mnemonic, operands = "braf", f"r{(word >> 8) & 0xF}"
        elif (word & 0xF0FF) == 0x400B:
            mnemonic, operands = "jsr", f"@r{(word >> 8) & 0xF}"
        elif (word & 0xF0FF) == 0x0003:
            mnemonic, operands = "bsrf", f"r{(word >> 8) & 0xF}"
        return dc_asm.Instruction(address, 2, raw, mnemonic, operands)

    return decode


def _decoded_shape(row: dict[str, str], refs: list[XrefCall],
                   all_lines: list[tuple[int, int, str]], data: bytes,
                   decode, roots: list[int]) -> DecodedShape:
    """Decode actual SH4 call sites and their CodeView statement groups.

    The raw xref graph's ``pool_refs`` column records every literal-pool
    address use.  Some are data offsets that collide with a procedure VA
    (army::do_attack's use of 0x198c was mislabeled InitAdvMenu).  The
    dossier control decoder follows only BSR/JSR call instructions.  Keeping
    each decoded site's breakpoint group also lets the gate preserve the
    attested source statement grouping and order rather than merely a set of
    helper names.
    """
    start, size = int(row["offset"], 0), int(row["cb"], 0)
    end = start + size
    lo, hi = bisect.bisect_left(roots, start), bisect.bisect_left(roots, end)
    blocks = dc_asm.build_cfg(start, end, decode, roots[lo:hi])
    instructions = {ins.address: ins for block in blocks
                    for ins in block.instructions}
    sh4 = dc_lines.Sh4(data)
    registers: dict[int, int] = {}
    calls: list[tuple[int, int]] = []
    for address, ins in sorted(instructions.items()):
        word = int.from_bytes(ins.data, "little")
        if (word >> 12) == 0xD:
            target = sh4.pool(address)
            if target is not None:
                registers[(word >> 8) & 0xF] = target
        target = None
        if (word & 0xF0FF) == 0x400B:
            target = registers.get((word >> 8) & 0xF)
        elif (word & 0xF000) == 0xB000:
            disp = word & 0xFFF
            if disp & 0x800:
                disp -= 0x1000
            target = dc_lines.POOL_BASE + address + 4 + disp * 2
        if target is not None and target >= dc_lines.POOL_BASE:
            calls.append((address, target - dc_lines.POOL_BASE))

    offsets = frozenset(target for _site, target in calls)
    names_by_offset = {ref.offset: ref.name for ref in refs}
    line_lo = bisect.bisect_left(all_lines, (start, -1, ""))
    line_hi = bisect.bisect_left(all_lines, (end, -1, ""))
    statements = all_lines[line_lo:line_hi]
    statement_addresses = [address for address, _line, _source in statements]
    grouped: list[list[str]] = [[] for _row in statements]
    for site, target in calls:
        index = bisect.bisect_right(statement_addresses, site) - 1
        name = names_by_offset.get(target)
        if index >= 0 and name is not None and _helper_token(name) is not None:
            grouped[index].append(name)
    groups = tuple(CallGroup(statements[index][1], tuple(callees))
                   for index, callees in enumerate(grouped) if callees)
    return DecodedShape(offsets, groups)


def _current_functions() -> dict[int, tuple[str, str]]:
    if not status.REPORT.is_file():
        return {}
    report = json.loads(status.REPORT.read_text())
    rvas = status.function_rvas()
    return {rvas[key]: key for key in status.fn_fuzzy(report) if key in rvas}


def _current_exact_vas() -> set[int]:
    if not status.REPORT.is_file():
        return set()
    report = json.loads(status.REPORT.read_text())
    scores = status.fn_fuzzy(report)
    rvas = status.function_rvas()
    return {common.IMAGE_BASE + rvas[key] for key, score in scores.items()
            if key in rvas and score >= 100.0}


def _line_starts(text: str) -> list[int]:
    starts = [0]
    starts.extend(match.end() for match in re.finditer("\n", text))
    return starts


def _body_after_claim(masked: str, starts: list[int], claim_line: int):
    """Body braces for the definition immediately following a VA claim.

    VA claims are definition-site annotations by contract.  Starting at the
    following line avoids a whole-TU name search per function, while the
    shared lexical/preprocessor mask ensures braces in comments, literals and
    carcass blocks cannot be selected.
    """
    start = starts[claim_line] if claim_line < len(starts) else len(masked)
    body_open = masked.find("{", start)
    if body_open < 0:
        return None
    body_close = _source._match_paren(masked, body_open, "{", "}")
    return (body_open, body_close) if body_close is not None else None


def _definitions_between(masked: str, fn: str, start: int, end: int):
    """Active definitions of ``fn`` inside one provenance interval.

    The shared TU mask is supplied by the caller.  Calling the public source
    locator here would rebuild that full-file mask once per Dreamcast row and
    turn this build gate from seconds into a minute.
    """
    names = _source.source_names(fn)
    # Corpus names are human-readable (`army::GetName`), whereas a method
    # defined inside its class body is necessarily spelled only `GetName`.
    # The VC6 locator's decorated-symbol path already supplies that fallback;
    # add it explicitly for these undecorated CodeView identities.
    basename = fn.rsplit("::", 1)[-1]
    if basename not in names:
        names.append(basename)
    for name in names:
        boundary = r"(?<![\w:~])" if "::" in name else r"(?<![\w~])"
        pattern = re.compile(boundary + re.escape(name))
        found = [definition for match in pattern.finditer(masked, start, end)
                 if (definition := _source._definition_at(
                     masked, match.start(), name)) is not None
                 and definition.head < end]
        if found:
            return found
    return []


def scan() -> tuple[
        int, list[MissingCall | MissingDefinition | MisgroupedCalls |
                  ContractViolation | FileContractViolation],
        set[AuditScope], set[str]]:
    """Return audited definitions, defects, scopes and proved DC-only facts."""
    current = _current_functions()
    exact_vas = _current_exact_vas()
    calls = _xref_calls()
    corpus = dreamcast.Corpus()
    checked = 0
    missing: list[MissingCall | MissingDefinition | MisgroupedCalls |
                  ContractViolation | FileContractViolation] = []
    audited: set[AuditScope] = set()
    dc_only: set[str] = set()
    cache: dict[str, tuple[str, str, list[int]]] = {}
    decoded_cache: dict[tuple[str, int], DecodedShape] = {}
    decoder = None

    def decoded(key: tuple[str, int], refs: list[XrefCall]) -> DecodedShape:
        nonlocal decoder
        if decoder is None:
            dump = dc_lines._dump_lines()
            data = dreamcast._gate_dc_exe(dc_lines.EXE)
            all_lines = _all_line_rows(dump)
            decoder = all_lines, data, _decode_sh4_control(data), \
                sorted({address for address, _line, _source in all_lines})
        if key not in decoded_cache:
            all_lines, data, decode, roots = decoder
            row = corpus.by_key.get(key)
            decoded_cache[key] = _decoded_shape(
                row, refs, all_lines, data, decode, roots) if row is not None \
                else DecodedShape(frozenset(), ())
        return decoded_cache[key]

    def attested(key: tuple[str, int], refs: list[XrefCall]) -> set[str]:
        if not any(ref.pool_refs for ref in refs):
            return _attested_names(refs, set())
        return _attested_names(refs, set(decoded(key, refs).offsets))

    def group_defect(key: tuple[str, int], refs: list[XrefCall], body: str,
                     va: int | None) \
            -> CallGroup | None:
        ordinary_calls = sum(ref.pool_refs + ref.bsr_calls for ref in refs
                             if _helper_token(ref.name) is not None)
        if ordinary_calls < 2:
            return None
        groups = groups_without_transfers(
            decoded(key, refs).groups,
            lambda callee: transferred(key, callee, body))
        helpers, descriptions = proven_dc_only_order_helpers(
            key, body, va, exact_vas)
        if helpers:
            groups = groups_without_helpers(groups, helpers)
            dc_only.update(descriptions)
        return misgrouped_from_body(body, groups)

    def add_contract_defects(va: int | None, key: tuple[str, int],
                             source: str, line: int, caller: str,
                             body: str) -> None:
        for rule in contract_violations(body, key):
            missing.append(ContractViolation(
                va, key[0], key[1], source, line, caller, rule))

    def transferred(key: tuple[str, int], callee: str,
                    caller_body: str) -> bool:
        transfer = PROVEN_CALL_TRANSFERS.get((key[0], key[1], callee))
        if transfer is None:
            return False
        path = common.HOMM3_DIR / transfer.receiver_path
        text = path.read_text(errors="replace")
        definitions = _source.find_definitions(text, transfer.receiver_name)
        if len(definitions) != 1:
            return False
        definition = definitions[0]
        receiver_body = text[
            definition.body_open + 1:definition.body_close]
        return transfer_satisfied(
            transfer, caller_body, receiver_body, exact_vas)

    seen_keys: set[tuple[str, int]] = set()
    for claim in dreamcast._source_claims():
        rva = claim.va - common.IMAGE_BASE
        identity = current.get(rva)
        key = claim.module, claim.dc_offset
        refs = calls.get(key)
        if identity is None or not refs:
            continue
        seen_keys.add(key)
        if claim.path not in cache:
            text = (common.HOMM3_DIR / claim.path).read_text(errors="replace")
            cache[claim.path] = text, _source.mask(text), _line_starts(text)
        text, masked, starts = cache[claim.path]
        span = _body_after_claim(masked, starts, claim.line)
        if span is None:
            continue
        checked += 1
        audited.add(_dc_audit_scope(key))
        body = text[span[0] + 1:span[1]]
        shape_body = apply_proven_call_spellings(
            key, body, claim.va, exact_vas)
        preliminary = missing_from_body(
            shape_body, [ref.name for ref in refs], key)
        names = attested(key, refs) if preliminary else set()
        body_missing = False
        for callee, helper in preliminary:
            if callee not in names:
                continue
            if transferred(key, callee, body):
                continue
            body_missing = True
            missing.append(MissingCall(
                claim.va, claim.module, claim.dc_offset, claim.path,
                claim.line, identity[1], callee, helper))
        if not body_missing and (group := group_defect(
                key, refs, shape_body, claim.va)):
            missing.append(MisgroupedCalls(
                claim.va, claim.module, claim.dc_offset, claim.path,
                claim.line, identity[1], group))
        add_contract_defects(claim.va, key, claim.path, claim.line,
                             identity[1], body)

    # /Ob2 can fold every copy of a restored helper, leaving no retail VA to
    # claim.  Its Dreamcast provenance line still identifies an active source
    # definition whose own helper graph must remain intact.  Without this
    # second pass, flattening CheckLuck's get_controller call would pass just
    # because do_attack continued to spell CheckLuck.
    provenance_rows: dict[tuple[str, str, int], list[dict[str, str]]] = {}
    for row in corpus.functions:
        key = corpus.key(row)
        if key in seen_keys or key not in calls:
            continue
        source = row.get("file", "").replace("/", "\\").lower()
        provenance_rows.setdefault(
            (row["module"], source, int(row["line"])), []).append(row)

    for path in sorted((common.HOMM3_DIR / "src").glob("*.cpp")):
        relpath = str(path.relative_to(common.HOMM3_DIR))
        if relpath not in cache:
            text = path.read_text(errors="replace")
            cache[relpath] = text, _source.mask(text), _line_starts(text)
        text, masked, _ = cache[relpath]
        marks = list(PROVENANCE_RE.finditer(text))
        for index, mark in enumerate(marks):
            source = mark.group(1).replace("/", "\\").lower()
            original_line = int(mark.group(2))
            rows = provenance_rows.get(
                (path.stem + ".obj", source, original_line), ())
            if not rows:
                continue
            end = marks[index + 1].start() if index + 1 < len(marks) \
                else len(text)
            for row in rows:
                key = corpus.key(row)
                definitions = _definitions_between(
                    masked, row["name"], mark.end(), end)
                if not definitions:
                    continue
                checked += 1
                audited.add(_dc_audit_scope(key))
                definition = definitions[0]
                body = text[definition.body_open + 1:definition.body_close]
                refs = calls[key]
                preliminary = missing_from_body(
                    body, [ref.name for ref in refs], key)
                names = attested(key, refs) if preliminary else set()
                body_missing = False
                for callee, helper in preliminary:
                    if callee not in names:
                        continue
                    body_missing = True
                    missing.append(MissingCall(
                        None, key[0], key[1], relpath,
                        text.count("\n", 0, definition.head) + 1,
                        row["name"], callee, helper))
                if not body_missing and (group := group_defect(
                        key, refs, body, None)):
                    missing.append(MisgroupedCalls(
                        None, key[0], key[1], relpath,
                        text.count("\n", 0, definition.head) + 1,
                        row["name"], group))
                add_contract_defects(
                    None, key, relpath,
                    text.count("\n", 0, definition.head) + 1,
                    row["name"], body)

    # The most important no-call header fact has no outgoing xref and thus no
    # ordinary row for the passes above: Dreamcast's army::Is accepts a mask,
    # not a bit index. Audit its definition and every reconstructed constant
    # call directly so a high-scoring shift helper cannot return unnoticed.
    is_paths = [common.HOMM3_DIR / "include/army.h"]
    is_paths.extend(sorted((common.HOMM3_DIR / "src").glob("*.cpp")))
    for path in is_paths:
        relpath = str(path.relative_to(common.HOMM3_DIR))
        audited.add(_file_audit_scope(relpath))
        text = path.read_text(errors="replace")
        defects = army_is_contract_violations(
            text, header=relpath == "include/army.h")
        if relpath == "include/army.h":
            defects.extend(army_header_roster_violations(text))
            defects.extend(army_class_roster_violations(text))
            checked += 3
        missing.extend(FileContractViolation(relpath, line, description)
                       for line, description in defects)

    cmbtmgr_header = common.HOMM3_DIR / "include/cmbtmgr.h"
    cmbtmgr_text = cmbtmgr_header.read_text(errors="replace")
    audited.add(_file_audit_scope("include/cmbtmgr.h"))
    cmbtmgr_defects = combat_manager_header_violations(cmbtmgr_text)
    checked += 1
    missing.extend(FileContractViolation("include/cmbtmgr.h", line,
                                         description)
                   for line, description in cmbtmgr_defects)

    struct_header = common.HOMM3_DIR / "include/struct.h"
    struct_text = struct_header.read_text(errors="replace")
    audited.add(_file_audit_scope("include/struct.h"))
    type_point_defects = type_point_header_violations(struct_text)
    checked += 1
    missing.extend(FileContractViolation("include/struct.h", line,
                                         description)
                   for line, description in type_point_defects)

    hero_header = common.HOMM3_DIR / "include/hero.h"
    hero_text = hero_header.read_text(errors="replace")
    audited.add(_file_audit_scope("include/hero.h"))
    get_target_defects = hero_get_target_header_violations(hero_text)
    get_location_defects = hero_get_location_header_violations(hero_text)
    checked += 2
    missing.extend(FileContractViolation("include/hero.h", line,
                                         description)
                   for line, description in get_target_defects)
    missing.extend(FileContractViolation("include/hero.h", line,
                                         description)
                   for line, description in get_location_defects)

    event_record_source = common.HOMM3_DIR / "src/event_record.cpp"
    event_record_text = event_record_source.read_text(errors="replace")
    audited.add(_file_audit_scope("src/event_record.cpp"))
    event_record_defects = event_record_constructor_violations(
        event_record_text)
    checked += 4
    missing.extend(FileContractViolation("src/event_record.cpp", line,
                                         description)
                   for line, description in event_record_defects)

    recruit_source = common.HOMM3_DIR / "src/recruit.cpp"
    recruit_source_text = recruit_source.read_text(errors="replace")
    recruit_header = common.HOMM3_DIR / "include/recruit.h"
    recruit_header_text = recruit_header.read_text(errors="replace")
    message_header = common.HOMM3_DIR / "include/message.h"
    message_header_text = message_header.read_text(errors="replace")
    audited.add(_file_audit_scope("src/recruit.cpp"))
    recruit_inline_defects = recruit_inline_contract_violations(
        recruit_source_text, recruit_header_text, message_header_text)
    checked += 2
    missing.extend(FileContractViolation("src/recruit.cpp", line,
                                         description)
                   for line, description in recruit_inline_defects)

    netmsg_header = common.HOMM3_DIR / "include/netmsg.h"
    netmsg_text = netmsg_header.read_text(errors="replace")
    audited.add(_file_audit_scope("include/netmsg.h"))
    hide_hero_defects = cmc_hide_hero_header_violations(netmsg_text)
    claim_message_defects = cmc_claim_header_violations(netmsg_text)
    checked += 6
    missing.extend(FileContractViolation("include/netmsg.h", line,
                                         description)
                   for line, description in
                   hide_hero_defects + claim_message_defects)

    game_header = common.HOMM3_DIR / "include/game.h"
    game_text = game_header.read_text(errors="replace")
    audited.add(_file_audit_scope("include/game.h"))
    get_hero_defects = game_get_hero_header_violations(game_text)
    computer_team_defects = game_is_computer_team_header_violations(game_text)
    randomize_header_defects = game_randomize_header_violations(game_text)
    checked += 3
    missing.extend(FileContractViolation("include/game.h", line,
                                         description)
                   for line, description in
                   get_hero_defects + computer_team_defects
                   + randomize_header_defects)

    split_header = common.HOMM3_DIR / "include/armygrp_split.h"
    split_text = split_header.read_text(errors="replace")
    audited.add(_file_audit_scope("include/armygrp_split.h"))
    split_defects = split_window_header_violations(split_text)
    checked += 1
    missing.extend(FileContractViolation("include/armygrp_split.h", line,
                                         description)
                   for line, description in split_defects)

    armygrp_header = common.HOMM3_DIR / "include/armygrp.h"
    armygrp_header_text = armygrp_header.read_text(errors="replace")
    armygrp_source = common.HOMM3_DIR / "src/armygrp.cpp"
    armygrp_source_text = armygrp_source.read_text(errors="replace")
    audited.add(_file_audit_scope("include/armygrp.h"))
    undead_defects = armygroup_has_some_undead_violations(
        armygrp_header_text, armygrp_source_text)
    const_query_defects = armygroup_const_query_violations(
        armygrp_header_text, armygrp_source_text)
    checked += 20
    missing.extend(FileContractViolation("include/armygrp.h", line,
                                         description)
                   for line, description in
                   undead_defects + const_query_defects)

    window_header = common.HOMM3_DIR / "include/window.h"
    window_text = window_header.read_text(errors="replace")
    audited.add(_file_audit_scope("include/window.h"))
    window_defects = window_header_violations(window_text)
    checked += 1
    missing.extend(FileContractViolation("include/window.h", line,
                                         description)
                   for line, description in window_defects)

    selection_header = (common.HOMM3_DIR
                        / "include/singleselectionwindow_priv.h")
    selection_text = selection_header.read_text(errors="replace")
    selection_source = common.HOMM3_DIR / "src/singleselectionwindow.cpp"
    selection_source_text = selection_source.read_text(errors="replace")
    selection_relpath = "include/singleselectionwindow_priv.h"
    audited.add(_file_audit_scope(selection_relpath))
    selection_defects = single_selection_window_contract_violations(
        selection_text, selection_source_text)
    checked += 1
    missing.extend(FileContractViolation(selection_relpath, line,
                                         description)
                   for line, description in selection_defects)

    netplayer_header = common.HOMM3_DIR / "include/netplayer.h"
    netplayer_text = netplayer_header.read_text(errors="replace")
    netplayer_relpath = "include/netplayer.h"
    audited.add(_file_audit_scope(netplayer_relpath))
    netplayer_defects = netplayer_constructor_contract_violations(
        netplayer_text, selection_source_text)
    checked += 1
    missing.extend(FileContractViolation(netplayer_relpath, line,
                                         description)
                   for line, description in netplayer_defects)

    selection_public_header = (common.HOMM3_DIR
                               / "include/singleselectionwindow.h")
    selection_public_text = selection_public_header.read_text(errors="replace")
    selection_public_relpath = "include/singleselectionwindow.h"
    audited.add(_file_audit_scope(selection_public_relpath))
    selection_signature_defects = update_player_pos_signature_violations(
        selection_public_text, selection_source_text)
    checked += 1
    missing.extend(FileContractViolation(selection_public_relpath, line,
                                         description)
                   for line, description in selection_signature_defects)

    # Header inlines are source bodies, not TU-local compiler knobs. Index
    # them by original CodeView location: one body may be instantiated in
    # several objects, while no x86 out-of-line copy need survive. An exact
    # provenance marker promises a definition, so replacing that definition
    # with a declaration is itself a fatal source-shape defect.
    header_rows: dict[tuple[str, int], list[dict[str, str]]] = {}
    for row in corpus.functions:
        key = corpus.key(row)
        if key not in calls:
            continue
        source = row.get("file", "").replace("/", "\\").lower()
        header_rows.setdefault((source, int(row["line"])), []).append(row)

    for path in sorted((common.HOMM3_DIR / "include").rglob("*.h")):
        relpath = str(path.relative_to(common.HOMM3_DIR))
        if relpath not in cache:
            text = path.read_text(errors="replace")
            cache[relpath] = text, _source.mask(text), _line_starts(text)
        text, masked, _ = cache[relpath]
        marks = list(PROVENANCE_RE.finditer(text))
        for index, mark in enumerate(marks):
            source = mark.group(1).replace("/", "\\").lower()
            original_line = int(mark.group(2))
            rows = header_rows.get((source, original_line), ())
            if not rows:
                continue
            end = marks[index + 1].start() if index + 1 < len(marks) \
                else len(text)
            seen_rows: set[tuple[str, int]] = set()
            for row in rows:
                key = corpus.key(row)
                if key in seen_rows:
                    continue
                seen_rows.add(key)
                refs = calls[key]
                names = attested(key, refs)
                enforced = [(callee, helper)
                            for callee in names
                            if (helper := _helper_token(callee)) is not None]
                if not enforced:
                    continue
                checked += 1
                audited.add(_dc_audit_scope(key))
                definitions = _definitions_between(
                    masked, row["name"], mark.end(), end)
                if not definitions:
                    missing.append(MissingDefinition(
                        key[0], key[1], relpath,
                        text.count("\n", 0, mark.start()) + 1, row["name"]))
                    continue
                definition = definitions[0]
                body = text[definition.body_open + 1:definition.body_close]
                body_missing = False
                for callee, helper in missing_from_body(
                        body, [callee for callee, _ in enforced],
                        key):
                    body_missing = True
                    missing.append(MissingCall(
                        None, key[0], key[1], relpath,
                        text.count("\n", 0, definition.head) + 1,
                        row["name"], callee, helper))
                if not body_missing and (group := group_defect(
                        key, refs, body, None)):
                    missing.append(MisgroupedCalls(
                        None, key[0], key[1], relpath,
                        text.count("\n", 0, definition.head) + 1,
                        row["name"], group))
                add_contract_defects(
                    None, key, relpath,
                    text.count("\n", 0, definition.head) + 1,
                    row["name"], body)

    def order(row: MissingCall | MissingDefinition | MisgroupedCalls |
              ContractViolation | FileContractViolation):
        if isinstance(row, FileContractViolation):
            return (True, 0, row.source, row.line, row.description)
        va = row.va if isinstance(
            row, (MissingCall, MisgroupedCalls, ContractViolation)) else None
        callee = row.callee if isinstance(row, MissingCall) else ""
        return (va is None, va or 0, row.dc_module, row.dc_offset, callee)

    return checked, sorted(missing, key=order), audited, dc_only


def violation_key(row: Violation) -> tuple[str, str, str, str]:
    """Stable ratchet identity, deliberately independent of retail score.

    Source paths and current line numbers move during reconstruction, and
    retail labels can be promoted without changing the Dreamcast source
    fact.  The Dreamcast module/offset plus callee or statement group is the
    durable identity for graph defects; direct file contracts use their file
    and description because they have no outgoing Dreamcast edge.
    """
    if isinstance(row, FileContractViolation):
        return "file-contract", row.source, "-", row.description
    location = f"0x{row.dc_offset:x}"
    if isinstance(row, MissingDefinition):
        return "definition", row.dc_module, location, "-"
    if isinstance(row, MissingCall):
        return "call", row.dc_module, location, row.callee
    if isinstance(row, MisgroupedCalls):
        detail = f"{row.group.line}:" + "|".join(row.group.callees)
        return "group", row.dc_module, location, detail
    return "contract", row.dc_module, location, row.rule.description


def violation_keys(rows: list[Violation]) -> set[tuple[str, str, str, str]]:
    return {violation_key(row) for row in rows}


def load_backlog() -> set[tuple[str, str, str, str]]:
    if not BASELINE.is_file():
        return set()
    out: set[tuple[str, str, str, str]] = set()
    for line_number, line in enumerate(BASELINE.read_text().splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        columns = line.split("\t")
        if len(columns) != 4:
            raise ValueError(
                f"{BASELINE.name}:{line_number}: expected four columns")
        out.add((columns[0], columns[1], columns[2], columns[3]))
    return out


def write_backlog_keys(keys: set[tuple[str, str, str, str]]) -> None:
    head = (
        "# KNOWN Dreamcast source-shape backlog. Each row is one stable\n"
        "# caller/callee, statement-group, or direct-contract identity.\n"
        "# The build rolls rows DOWN-only; --write-baseline is the only\n"
        "# deliberate way to admit new debt. Retail score is never a waiver.\n"
        "# kind\tdc-module-or-source\tdc-offset\tcallee-or-contract\n")
    BASELINE.write_text(head + "".join(
        "\t".join(key) + "\n" for key in sorted(keys)))


def write_backlog(rows: list[Violation]) -> None:
    write_backlog_keys(violation_keys(rows))


def new_violations(rows: list[Violation],
                   backlog: set[tuple[str, str, str, str]]) \
        -> list[Violation]:
    """Return one diagnostic row for every unbaselined source fact."""
    out: list[Violation] = []
    seen: set[tuple[str, str, str, str]] = set()
    for row in rows:
        key = violation_key(row)
        if key not in backlog and key not in seen:
            out.append(row)
            seen.add(key)
    return out


def ratcheted_backlog(
        rows: list[Violation],
        backlog: set[tuple[str, str, str, str]],
        audited: set[AuditScope] | None = None) \
        -> set[tuple[str, str, str, str]]:
    """Down-only baseline after audited, currently restored facts disappear.

    ``audited`` is mandatory for a red-gate write: a missing definition or
    temporarily unresolvable claim must not look like a restoration merely
    because its old violation row was never reached by the scan.
    """
    current = violation_keys(rows)
    if audited is None:
        return backlog & current
    return {key for key in backlog
            if key in current or _key_audit_scope(key) not in audited}


def render(row: Violation) -> str:
    if isinstance(row, FileContractViolation):
        return (f"DC SOURCE SHAPE: {row.source}:{row.line} violates "
                f"source contract: {row.description}")
    if isinstance(row, MissingDefinition):
        return (f"DC SOURCE SHAPE: {row.dc_module}:dc:0x{row.dc_offset:x} "
                f"{row.caller} has no active definition after provenance "
                f"marker {row.source}:{row.line}")
    if isinstance(row, MisgroupedCalls):
        location = f"0x{row.va:08x}" if row.va is not None else \
            f"{row.dc_module}:dc:0x{row.dc_offset:x}"
        helpers = ", ".join(
            helper for callee in row.group.callees
            if (helper := _helper_token(callee)) is not None)
        return (f"DC SOURCE SHAPE: {location} {row.caller} breaks attested "
                f"statement order/group at DC source line {row.group.line} "
                f"({helpers}) in {row.source}:{row.line}")
    if isinstance(row, ContractViolation):
        location = f"0x{row.va:08x}" if row.va is not None else \
            f"{row.dc_module}:dc:0x{row.dc_offset:x}"
        return (f"DC SOURCE SHAPE: {location} {row.caller} violates "
                f"source contract: {row.rule.description} "
                f"({row.source}:{row.line})")
    location = f"0x{row.va:08x}" if row.va is not None else \
        f"{row.dc_module}:dc:0x{row.dc_offset:x}"
    return (f"DC SOURCE SHAPE: {location} {row.caller} omits attested "
            f"{row.callee} (expected `{row.helper}(...)` in "
            f"{row.source}:{row.line})")


def _ratchet_gate(checked: int, missing: list[Violation],
                  audited: set[AuditScope], dc_only: set[str]) -> list[str]:
    try:
        backlog = load_backlog()
    except ValueError as error:
        return [f"dc-source-shape BASELINE BROKEN: {error}"]
    current = violation_keys(missing)
    ratcheted = ratcheted_backlog(missing, backlog, audited)
    stale = backlog - ratcheted
    if stale:
        # Down-only even on a red build: removing an absent backlog identity
        # cannot bless any fresh defect, and delaying this write would let an
        # unrelated dirty file keep a restored helper eligible for flattening.
        write_backlog_keys(ratcheted)
    fresh = new_violations(missing, backlog)
    if not fresh:
        summary = (f"[build] dc-source-shape: {checked} DC source "
                   f"definitions; {len(current)} known-backlog defect(s)")
        if dc_only:
            summary += f"; {len(dc_only)} proved dc-only order fact(s)"
        if stale:
            summary += f"; ratcheted {len(stale)} retired row(s)"
        print(summary + " - no new source flattening")
        return []
    shown = [render(row) for row in fresh[:BUILD_REPORT_LIMIT]]
    remainder = len(fresh) - len(shown)
    if remainder:
        shown.append(
            f"DC SOURCE SHAPE: {remainder} additional NEW defect(s) hidden; "
            "run `python3 -m homm3.match.dc_source_shape --backlog` for "
            "the full ratchet report")
    summary = (f"DC SOURCE SHAPE RATCHET: {len(fresh)} new defect(s); retail "
               "percentage cannot waive an attested helper boundary")
    if dc_only:
        summary += f"; {len(dc_only)} separate order fact(s) proved dc-only"
    if stale:
        summary += f"; ratcheted {len(stale)} independently retired row(s)"
    shown.append(summary)
    return shown


def run_gate() -> list[str]:
    broken = selftest()
    if broken:
        return [f"dc-source-shape SELFTEST BROKEN: {item}" for item in broken]
    return _ratchet_gate(*scan())


def main(argv=None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if "--selftest" in argv:
        broken = selftest()
        for item in broken:
            print(f"SELFTEST BROKEN: {item}", file=sys.stderr)
        print("selftest OK" if not broken else "selftest FAILED")
        return 2 if broken else 0
    broken = selftest()
    if broken:
        for item in broken:
            print(f"SELFTEST BROKEN: {item}", file=sys.stderr)
        return 2
    checked, missing, audited, dc_only = scan()
    if "--write-baseline" in argv:
        write_backlog(missing)
        print(f"Dreamcast source-shape backlog frozen: "
              f"{len(violation_keys(missing))} row(s) -> "
              f"{BASELINE.relative_to(common.HOMM3_DIR)}")
        return 0
    try:
        backlog = load_backlog()
    except ValueError as error:
        print(f"BASELINE BROKEN: {error}", file=sys.stderr)
        return 2
    if "--backlog" in argv:
        current = violation_keys(missing)
        for row in missing:
            tag = "known" if violation_key(row) in backlog else "NEW  "
            print(f"{tag} {render(row)}")
        for key in sorted(backlog - current):
            print("stale DC SOURCE SHAPE: " + "\t".join(key))
        for description in sorted(dc_only):
            print("dc-only DC SOURCE SHAPE: " + description)
        fresh = new_violations(missing, backlog)
        print(f"checked {checked} DC source definitions; "
              f"{len(current & backlog)} known, {len(fresh)} new, "
              f"{len(backlog - current)} stale source-shape defect(s)")
        return 1 if fresh else 0
    fatal = _ratchet_gate(checked, missing, audited, dc_only)
    for line in fatal:
        print(line, file=sys.stderr)
    return 1 if fatal else 0


if __name__ == "__main__":
    sys.exit(main())
