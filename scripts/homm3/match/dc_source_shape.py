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


# Complete removed the Dreamcast zero-experience Random(0, 50) + 40 fallback
# from readHeroData.  Keep the admission tied to the decoded Complete flow:
# an absent custom-experience byte leaves zero in ``experience`` and that zero
# is passed unchanged to GetStartingHeroId.  The same bounded expression is a
# source contract below; neither a score nor the absence of a token is proof.
READ_HERO_COMPLETE_EXPERIENCE_RE = (
    r"bCustomExperience\s*=\s*experienceFlag\s*!=\s*0\s*;\s*"
    r"if\s*\(\s*bCustomExperience\s*\)\s*\{.*?"
    r"else\s*\{\s*experience\s*=\s*0\s*;\s*\}\s*\}\s*"
    r"if\s*\(\s*HeroID\s*==\s*-1\s*\)\s*\{.*?"
    r"HeroID\s*=\s*gpGame\s*->\s*GetStartingHeroId\s*\(\s*"
    r"alignment\s*,\s*char_buffer\s*,\s*experience\s*\)\s*;")

# Dreamcast stores the flag unconditionally and calls
# ``strncpy(hero_data->Name, tempText, 12)`` only for a non-random hero.
# Complete retail has one customName guard followed by a runtime NUL scan and
# strlen+1 copy; the current ``strcpy`` lowering matches that sequence
# instruction-for-instruction.  This expression is the bounded replacement
# proof for classifying the older imported helper and second guard as DC-only.
READ_HERO_COMPLETE_NAME_RE = (
    r"if\s*\(\s*customName\s*\)\s*\{\s*"
    r"hero_data\s*->\s*bCustomName\s*=\s*1\s*;\s*"
    r"strcpy\s*\(\s*hero_data\s*->\s*Name\s*,\s*tempText\s*\)\s*;"
    r"\s*\}")

# Dreamcast's WinCE dispatcher has controller-cursor helpers that the Complete
# Win32 retail dispatcher directly contradicts.  Keep the admission tied to
# the two decoded Complete replacement groups rather than to a missing symbol
# or a similarity score.  The modal group refreshes through ResetMouse after
# each desktop dialog.  The input group consumes mouse movement through
# PeekEvent/InCombatArea and exposes the Complete F5/F6/F7/F8, keypad, Faerie
# Dragon and army-view key roster instead of the WinCE cursor-navigation arm.
PROCESS_COMBAT_MSG_COMPLETE_MODAL_RE = (
    r"\A(?!.*\bFullUpdate\s*\().*?"
    r"case\s+TCombatWindow\s*::\s*COMBAT_RIGHT_COMMAND_0_ID\s*:\s*"
    r"if\s*\(\s*!\s*heroes\s*\[\s*currentSide\s*\]\s*\)\s*\{\s*"
    r"NormalDialog\s*\([^;]*\)\s*;\s*\}\s*else\s*\{\s*"
    r"InitiateSpell\s*\(\s*ViewSpells\s*\(\s*\)\s*,\s*0\s*\)\s*;\s*"
    r"ResetMouse\s*\(\s*\)\s*;\s*\}\s*break\s*;.*?"
    r"case\s+TCombatWindow\s*::\s*COMBAT_LEFT_COMMAND_1_ID\s*:.*?"
    r"NormalDialog\s*\([^;]*\)\s*;\s*"
    r"if\s*\(\s*gpWindowManager\s*->\s*dialogReturn\s*==\s*"
    r"DIALOG_RETURN_ACCEPT\s*\)\s*field_3c\s*=\s*4\s*;\s*"
    r"ResetMouse\s*\(\s*\)\s*;\s*break\s*;.*?"
    r"case\s+TCombatWindow\s*::\s*COMBAT_LEFT_COMMAND_0_ID\s*:.*?"
    r"if\s*\(\s*DoSurrender\s*\(\s*\)\s*\)\s*\{.*?\}\s*"
    r"ResetMouse\s*\(\s*\)\s*;\s*break\s*;")

PROCESS_COMBAT_MSG_COMPLETE_INPUT_RE = (
    r"\A(?!.*\b(?:InitMouse|MoveCursorMenu|MoveCursorTo|ScrollCombatArea)"
    r"\s*\().*?case\s+MESSAGE_MOUSE_MOVE\s*:\s*\{.*?"
    r"msgTemp\s*=\s*gpInputManager\s*->\s*PeekEvent\s*\(\s*\)\s*;.*?"
    r"if\s*\(\s*!?\s*InCombatArea\s*\(\s*mouseX\s*,\s*mouseY\s*\)\s*\)"
    r"\s*\{.*?case\s+MESSAGE_KEY_DOWN\s*:\s*"
    r"switch\s*\(\s*msg\.codeX\s*\)\s*\{\s*"
    r"case\s+KEYCODE_F5\s*:.*?WritePrefs\s*\(\s*\)\s*;\s*break\s*;"
    r".*?case\s+KEYCODE_F6\s*:.*?SetCombatGrid\s*\([^;]*\)\s*;\s*"
    r"break\s*;.*?case\s+KEYCODE_F7\s*:.*?SetCombatGrid\s*\([^;]*\)"
    r"\s*;\s*break\s*;.*?case\s+KEYCODE_F8\s*:.*?SetCombatGrid\s*\("
    r"[^;]*\)\s*;\s*break\s*;.*?case\s+KEYCODE_KP_MINUS\s*:\s*"
    r"combatWindow\s*->\s*scroll_rollover\s*\(\s*-1\s*\)\s*;\s*"
    r"break\s*;.*?case\s+KEYCODE_KP_2\s*:\s*combatWindow\s*->\s*"
    r"scroll_rollover\s*\(\s*1\s*\)\s*;\s*break\s*;.*?"
    r"case\s+KEYCODE_F\s*:.*?\bCREATURE_FAERIE_DRAGON\b.*?"
    r"InitiateSpell\s*\([^;]*\)\s*;.*?break\s*;.*?"
    r"case\s+KEYCODE_T\s*:.*?ViewArmy\s*\(\s*get_current_army\s*\("
    r"\s*\)\s*,\s*0\s*\)\s*;\s*ResetMouse\s*\(\s*\)\s*;")

PROCESS_COMBAT_MSG_COMPLETE_OUTSIDE_MOUSE_RE = (
    r"\A(?!.*\bConvertToHover\s*\(.*\bConvertToHover\s*\().*?"
    r"int\s+gridIndex\s*=\s*GetGridIndex\s*\(\s*mouseX\s*,\s*mouseY"
    r"\s*\)\s*;\s*UpdateMouseGrid\s*\(\s*gridIndex\s*,\s*0\s*\)"
    r"\s*;\s*if\s*\(\s*!\s*InCombatArea\s*\(\s*mouseX\s*,\s*mouseY"
    r"\s*\)\s*\)\s*\{\s*TurnOffHighlighter\s*\(\s*1\s*\)\s*;.*?"
    r"gpWindowManager\s*->\s*ConvertToHover\s*\("
    r"\s*msg\s*\)\s*;\s*gpMouseManager\s*->\s*SetPointer\s*\(\s*6"
    r"\s*,\s*mouseManager\s*::\s*COMBAT_SET\s*\)\s*;\s*"
    r"field_132d4\s*=\s*-1\s*;\s*field_132dc\s*=\s*-99\s*;\s*"
    r"return\s+MESSAGE_DISPATCH_CONSUME\s*;\s*\}")

DO_SURRENDER_COMPLETE_RE = (
    r"\A(?!.*\bFullUpdate\s*\()\s*"
    r"gSurrenderCost695030\s*=\s*get_surrender_cost\s*\(\s*\)\s*;\s*"
    r"sprintf\s*\(\s*gText\s*,\s*gpGeneralText\s*->\s*GetText\s*\("
    r"\s*33\s*\)\s*,\s*heroes\s*\[\s*1\s*-\s*currentSide\s*\]\s*"
    r"->\s*name\s*,\s*gSurrenderCost695030\s*\)\s*;\s*"
    r"NormalDialog\s*\([^;]*\)\s*;\s*return\s+gpWindowManager\s*->\s*"
    r"dialogReturn\s*==\s*DIALOG_RETURN_ACCEPT\s*;\s*\Z")


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
class DefinitionOwner:
    description: str
    path: str
    name: str


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
class ProvenRevisionRemoval:
    description: str
    caller_va: int
    retail_pattern: str
    dc_only_helpers: tuple[str, ...]
    unclaimed_inline: bool = False


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


# A retained RVA-order claim can own a COMDAT whose live source definition is
# still the original class-body inline in a header.  Redirect only individually
# proved owners: an absent or ambiguous mapped definition remains a fatal
# definition defect, and its helper graph is audited like any other body.
PROVEN_DEFINITION_OWNERS: dict[tuple[str, int], DefinitionOwner] = {
    ("command.obj", 0x70AD0): DefinitionOwner(
        "CMessageKill's command.obj COMDAT is emitted from the original "
        "remote.h class-body inline reconstructed in netmsg.h",
        "include/netmsg.h", "CMessageKill::~CMessageKill"),
}


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
    ("resourcemanager.obj", 0x121EC8, "LODFile::read"):
        CallTransfer(
            "Complete GetPalette24 routes Dreamcast's two direct LODFile::read "
            "operations through the exact t_lod_file_adapter::Read receiver; "
            "retail vtable 0x641128 slot 1 targets 0x559110",
            "src/resourcemanager.cpp",
            "ResourceManager::t_lod_file_adapter::Read", 0x00559110,
            r"\bt_lod_file_adapter\s+stream\s*\(\s*lodFile\s*\)\s*;\s*"
            r"TAbstractFile\s*\*\s*streamInterface\s*=\s*&\s*stream\s*;\s*"
            r"streamInterface\s*->\s*Read\s*\(\s*header\s*,\s*"
            r"sizeof\s*\(\s*header\s*\)\s*\)\s*;\s*"
            r"streamInterface\s*->\s*Read\s*\(\s*rgba\s*,\s*"
            r"sizeof\s*\(\s*rgba\s*\)\s*\)\s*;",
            r"return\s+lod_file\s*->\s*read\s*\(\s*data\s*,\s*size\s*\)"
            r"\s*\?\s*0\s*:\s*size\s*;"),
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
    ("singleselectionwindow.obj", 0x135AA4): (
        CallSpelling(
            "Complete SetupNewGameMode replaces each direct AddNewPlayer "
            "with SetNewPlayerSlot; retail 0x58e700 begins by calling "
            "CNetPlayerHandler::AddNewPlayer on the supplied record before "
            "performing Complete's seat reconciliation",
            0x0057F740, "CNetPlayerHandler::AddNewPlayer",
            r"\bSetNewPlayerSlot(?=\s*\()", "AddNewPlayer"),
    ),
    ("singleselectionwindow.obj", 0x13575C): (
        CallSpelling(
            "Complete SetupLoadGameMode replaces each direct AddNewPlayer "
            "with SetNewPlayerSlot; exact retail 0x58e700 begins with the "
            "same CNetPlayerHandler::AddNewPlayer operation before Complete's "
            "seat reconciliation",
            0x0057F330, "CNetPlayerHandler::AddNewPlayer",
            r"\bSetNewPlayerSlot(?=\s*\()", "AddNewPlayer"),
    ),
}


# A non-exact Complete caller can still prove one revision spelling directly
# through a retail relocation or decoded body. Keep these separate from
# PROVEN_CALL_SPELLINGS: admission is per caller and the matching SOURCE_RULES
# entry must require the full expression shape, so this cannot become a name
# waiver.
NONEXACT_RETAIL_PROVEN_CALL_SPELLINGS: dict[
        tuple[str, int], tuple[CallSpelling, ...]] = {
    ("swapmgr.obj", 0x15D150): (
        CallSpelling(
            "Complete swapManager::handle_artifact_click replaces the "
            "Dreamcast free artifactAllowedInSlot helper with the retail "
            "relocation-proved hero::HeroFn_004E2840 member call",
            0x005AF590, "artifactAllowedInSlot",
            r"\bHeroFn_004E2840(?=\s*\()", "artifactAllowedInSlot"),
    ),
}


# A lexical helper name is normally the most the source-shape pass can prove:
# same-class methods may legitimately be called on a peer object.  Require a
# self receiver only for individually decoded SH4 call sites.  ShowWidget and
# all four SetupAdvancedOptions IsHost sites reload their saved ``this`` into
# r4 immediately before the jsr; pDPlay->IsHost() is therefore not that edge.
PROVEN_SELF_CALLS = frozenset({
    ("singleselectionwindow.obj", 0x135AA4,
     "TSingleSelectionWindow::IsHost"),
    ("singleselectionwindow.obj", 0x135DA8,
     "TSingleSelectionWindow::IsHost"),
    ("singleselectionwindow.obj", 0x136388,
     "TSingleSelectionWindow::IsHost"),
})


# A different lexical order is admitted only after the Complete caller was
# exact, the Dreamcast ordering was imposed and measured, and that older shape
# broke the exact lowering.  Keep the named helpers themselves mandatory; only
# their cross-statement order is classified DC-only while the exact caller and
# the bounded Complete source pattern both survive.
PROVEN_ORDER_SKEWS: dict[tuple[str, int], tuple[ProvenOrderSkew, ...]] = {
    ("singleselectionwindow.obj", 0x135F04): (
        ProvenOrderSkew(
            "Complete SetupScenarioOptions spells the host/header choice as "
            "SetupOrigData then GetHeaders; Dreamcast's inverse conditional "
            "places GetHeaders then SetupOrigData",
            0x00580A70,
            r"if\s*\(\s*bVideoPaused\s*&&\s*!\s*pDPlay\s*->\s*"
            r"IsHost\s*\(\s*\)\s*&&\s*!\s*m_flag65\s*\)\s*"
            r"gpGame\s*->\s*SetupOrigData\s*\(\s*\)\s*;\s*else\s*"
            r"GetHeaders\s*\(\s*&\s*HeadersA\s*\)\s*;",
            ("game::SetupOrigData",
             "TSingleSelectionWindow::GetHeaders")),
    ),
    ("singleselectionwindow.obj", 0x135AA4): (
        ProvenOrderSkew(
            "SetupNewGameMode's Complete client setup precedes the one "
            "retained host UI pass; Dreamcast also has an earlier duplicate "
            "GetHeaders / HighlightFile / SetCurrentMap pass",
            0x0057F740,
            r"\bgpGame\s*->\s*SetupOrigData\s*\(\s*\)\s*;.*?"
            r"if\s*\(\s*IsHost\s*\(\s*\)\s*\)\s*\{\s*"
            r"GetHeaders\s*\(\s*&\s*HeadersA\s*\)\s*;.*?"
            r"HighlightFile\s*\(.*?\)\s*;\s*"
            r"SetCurrentMap\s*\(\s*currentMap\s*,\s*false\s*\)\s*;",
            ("TSingleSelectionWindow::GetHeaders",
             "TSingleSelectionWindow::HighlightFile",
             "TSingleSelectionWindow::SetCurrentMap")),
    ),
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


# The same proof shape is useful before a large caller is byte-exact when the
# retail body itself directly fixes the replacement order.  Keep this table
# separate so exactness is never silently weakened for the older admissions.
RETAIL_BYTE_PROVEN_ORDER_SKEWS: dict[
        tuple[str, int], tuple[ProvenOrderSkew, ...]] = {
    ("command.obj", 0x6C070): (
        ProvenOrderSkew(
            "Complete ProcessCombatMsg has one outside-combat "
            "ConvertToHover/SetPointer group after GetGridIndex, "
            "UpdateMouseGrid and InCombatArea; Dreamcast also has an "
            "earlier WinCE screen-edge copy",
            0x00474D80, PROCESS_COMBAT_MSG_COMPLETE_OUTSIDE_MOUSE_RE,
            ("heroWindowManager::ConvertToHover",
             "mouseManager::SetPointer")),
    ),
}


# Complete can replace an entire older-revision statement group rather than
# merely reorder it. Classify such a call as DC-only only when the Complete
# caller is byte-exact and its bounded replacement source remains present.
# This is deliberately separate from order skew: helpers listed here are not
# mandatory in the reconstructed Complete body.
PROVEN_REVISION_REMOVALS: dict[
        tuple[str, int], tuple[ProvenRevisionRemoval, ...]] = {
    ("singleselectionwindow.obj", 0x135F04): (
        ProvenRevisionRemoval(
            "Complete SetupScenarioOptions replaces Dreamcast's "
            "loadGameMode && IsMultiPlayer widget-333..339 show/hide group "
            "with the exact Complete scenario-widget group",
            0x00580A70,
            r"\bShowWidget\s*\(\s*137\s*\)\s*;.*?"
            r"\bShowWidget\s*\(\s*141\s*\)\s*;.*?"
            r"\bShowWidget\s*\(\s*190\s*\)\s*;.*?"
            r"\bShowWidget\s*\(\s*195\s*\)\s*;",
            ("TSingleSelectionWindow::IsMultiPlayer", "widget::hide")),
    ),
}


# Retail can directly contradict one older Dreamcast helper before the whole
# caller is exact.  Admit that contradiction only when a bounded Complete
# source expression remains present; SOURCE_RULES independently requires the
# same expression and rejects the older helper, omission and flattening.  This
# is deliberately not a general name substitution or score-based waiver.
RETAIL_BYTE_PROVEN_REVISION_REMOVALS: dict[
        tuple[str, int], tuple[ProvenRevisionRemoval, ...]] = {
    ("command.obj", 0x6C070): (
        ProvenRevisionRemoval(
            "Complete ProcessCombatMsg replaces Dreamcast's WinCE modal "
            "FullUpdate calls with the decoded desktop dialog/ResetMouse "
            "group",
            0x00474D80, PROCESS_COMBAT_MSG_COMPLETE_MODAL_RE,
            ("combatManager::FullUpdate",)),
        ProvenRevisionRemoval(
            "Complete ProcessCombatMsg replaces Dreamcast's WinCE "
            "controller cursor helpers with the decoded mouse PeekEvent/"
            "InCombatArea path and Complete keyboard roster",
            0x00474D80, PROCESS_COMBAT_MSG_COMPLETE_INPUT_RE,
            ("combatManager::InitMouse", "combatManager::MoveCursorMenu",
             "combatManager::MoveCursorTo",
             "combatManager::ScrollCombatArea")),
    ),
    ("command.obj", 0x6E990): (
        ProvenRevisionRemoval(
            "Complete's inlined DoSurrender expansion ends at the dialog "
            "result and omits Dreamcast's following FullUpdate",
            0x00474D80, DO_SURRENDER_COMPLETE_RE,
            ("combatManager::FullUpdate",), unclaimed_inline=True),
    ),
    ("mapcell.obj", 0xF0DF4): (
        ProvenRevisionRemoval(
            "Complete readHeroData removes Dreamcast's zero-experience "
            "Random(0, 50) + 40 fallback; decoded retail passes zero "
            "unchanged to GetStartingHeroId and has no Random relocation",
            0x005021C0, READ_HERO_COMPLETE_EXPERIENCE_RE, ("Random",)),
        ProvenRevisionRemoval(
            "Complete readHeroData replaces Dreamcast's random-hero-guarded "
            "strncpy(Name, tempText, 12) with a single customName guard and "
            "an instruction-identical inlined strcpy",
            0x005021C0, READ_HERO_COMPLETE_NAME_RE, ("strncpy",)),
    ),
}


# Named calls cover most recoverable shape automatically. These bounded
# contracts preserve source-visible facts that disappear before the SH4 xref
# graph: inlined accessors/operators, a source order hidden by scheduling, and
# nesting within a single attested statement group.
SOURCE_RULES: dict[tuple[str, int], tuple[SourceRule, ...]] = {
    ("singleselectionwindow.obj", 0x1304A8): (
        SourceRule(
            "CNetPlayerHandler::SetNextPlayer keeps Dreamcast's bounded i "
            "scan, inlined IsHuman helper boundary and early-success body",
            r"int\s+i\s*=\s*start\s*;\s*"
            r"while\s*\(\s*i\s*<\s*MAX_PLAYERS\s*\)\s*\{\s*"
            r"if\s*\(\s*humanPlayers\s*\[\s*i\s*\]\.IsHuman\s*"
            r"\(\s*\)\s*\)\s*\{\s*"
            r"assignedPos\s*=\s*humanPlayers\s*\[\s*i\s*\]\.playerPos"
            r"\s*;\s*humanPlayers\s*\[\s*i\s*\]\.playerPos\s*=\s*pos"
            r"\s*;\s*humanPlayers\s*\[\s*i\s*\]\.heroIndex\s*=\s*-1"
            r"\s*;\s*humanPlayers\s*\[\s*i\s*\]\.townIndex\s*=\s*-1"
            r"\s*;\s*return\s+1\s*;\s*\}\s*\+\+\s*i\s*;\s*\}\s*"
            r"return\s+1\s*;"),
    ),
    ("singleselectionwindow.obj", 0x12F9C8): (
        SourceRule(
            "BackupGameHeaders keeps Dreamcast's sole int i local and the "
            "shared header/setup/campaign/scalar assignment order",
            r"\A\s*int\s+i\s*;\s*"
            r"dest\s*->\s*mapHeader\s*=\s*src\s*->\s*mapHeader\s*;\s*"
            r"dest\s*->\s*setup\s*=\s*src\s*->\s*setup\s*;\s*"
            r"dest\s*->\s*campaign\s*=\s*src\s*->\s*campaign\s*;\s*"
            r"dest\s*->\s*field_1f4d4\s*=\s*src\s*->\s*field_1f4d4\s*;\s*"
            r"dest\s*->\s*difficultyRating\s*=\s*"
            r"src\s*->\s*difficultyRating\s*;\s*"
            r"dest\s*->\s*field_1f635\s*=\s*src\s*->\s*field_1f635\s*;"),
        SourceRule(
            "BackupGameHeaders keeps Dreamcast's two scoped current-player "
            "arms in their attested direction",
            r"if\s*\(\s*src\s*==\s*gpGame\s*\)\s*\{\s*"
            r"saveCurPlayer\s*=\s*gNetLocalGamePos\s*;\s*\}\s*"
            r"else\s*\{\s*gNetLocalGamePos\s*=\s*saveCurPlayer\s*;\s*\}"),
        SourceRule(
            "BackupGameHeaders keeps Complete's byte-exact Dinkumware "
            "std::copy player transfer and the three ordered array copies; "
            "the player std::copy is retail-only while preserving the "
            "Dreamcast playerData assignment operation",
            r"std\s*::\s*copy\s*\(\s*src\s*->\s*players\s*,\s*"
            r"src\s*->\s*players\s*\+\s*8\s*,\s*dest\s*->\s*players\s*"
            r"\)\s*;\s*"
            r"std\s*::\s*copy\s*\(\s*src\s*->\s*heroAvailability\s*,\s*"
            r"src\s*->\s*heroAvailability\s*\+\s*sizeof\s*\(\s*"
            r"src\s*->\s*heroAvailability\s*\)\s*,\s*"
            r"dest\s*->\s*heroAvailability\s*\)\s*;\s*"
            r"std\s*::\s*copy\s*\(\s*src\s*->\s*saveFileName\s*,\s*"
            r"src\s*->\s*saveFileName\s*\+\s*sizeof\s*\(\s*"
            r"src\s*->\s*saveFileName\s*\)\s*,\s*"
            r"dest\s*->\s*saveFileName\s*\)\s*;\s*"
            r"std\s*::\s*copy\s*\(\s*src\s*->\s*playerDisabled\s*,\s*"
            r"src\s*->\s*playerDisabled\s*\+\s*sizeof\s*\(\s*"
            r"src\s*->\s*playerDisabled\s*\)\s*,\s*"
            r"dest\s*->\s*playerDisabled\s*\)\s*;"),
    ),
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
    ("command.obj", 0x6AF98): (
        SourceRule(
            "automate_catapult keeps all three Dreamcast "
            "valid_wall_target boundaries",
            r"\bvalid_wall_target\s*\(", 3, 3),
        SourceRule(
            "automate_catapult keeps the three Dreamcast "
            "get_wall_strength boundaries",
            r"\bget_wall_strength\s*\(", 3, 3),
        SourceRule(
            "automate_catapult keeps its sole Dreamcast SRandom boundary",
            r"\bSRandom\s*\(", 1, 1),
        SourceRule(
            "automate_catapult keeps its sole Dreamcast "
            "get_secondary_skill boundary",
            r"\bget_secondary_skill\s*\(", 1, 1),
        SourceRule(
            "automate_catapult keeps Dreamcast's final command, direct "
            "wall-target hex store, field_40 clear and return order",
            r"\bfield_3c\s*=\s*9\s*;\s*"
            r"field_44\s*=\s*wallTargets\s*\[\s*target\s*\]\s*\.\s*"
            r"target_hex\s*;\s*field_40\s*=\s*-\s*1\s*;\s*return\s+1\s*;"),
    ),
    ("command.obj", 0x6B12C): (
        SourceRule(
            "automate_first_aid_tent keeps Dreamcast's final command, "
            "direct target-grid store, field_40 clear and return statement "
            "order without an artificial grid local",
            r"\bfield_3c\s*=\s*11\s*;\s*"
            r"field_44\s*=\s*armies\s*\[\s*side\s*\]\s*"
            r"\[\s*best_index\s*\]\s*\.\s*gridIndex\s*;\s*"
            r"field_40\s*=\s*-\s*1\s*;\s*return\s+1\s*;"),
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
    ("command.obj", 0x6C070): (
        SourceRule(
            "ProcessCombatMsg keeps Complete's retail-decoded desktop "
            "modal/ResetMouse group and does not restore WinCE FullUpdate",
            PROCESS_COMBAT_MSG_COMPLETE_MODAL_RE),
        SourceRule(
            "ProcessCombatMsg keeps Complete's retail-decoded mouse and "
            "keyboard replacement for WinCE controller cursor navigation",
            PROCESS_COMBAT_MSG_COMPLETE_INPUT_RE),
        SourceRule(
            "ProcessCombatMsg keeps Complete's sole outside-combat hover "
            "group after GetGridIndex/UpdateMouseGrid/InCombatArea",
            PROCESS_COMBAT_MSG_COMPLETE_OUTSIDE_MOUSE_RE),
    ),
    ("command.obj", 0x6E990): (
        SourceRule(
            "DoSurrender keeps the Dreamcast helper boundary around the "
            "Complete retail expansion and its byte-proven no-FullUpdate "
            "tail",
            DO_SURRENDER_COMPLETE_RE),
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
    ("philai.obj", 0x10E9A8): (
        SourceRule(
            "move_hero keeps Dreamcast's max helper with the unsigned "
            "field_041 addend and retail-decoded 1000-point floor",
            r"max_distance\s*=\s*max\s*\(\s*"
            r"current_hero\s*->\s*movePoints\s*\+\s*"
            r"static_cast\s*<\s*unsigned\s+short\s*>\s*\(\s*"
            r"current_hero\s*->\s*field_041\s*\)\s*\+\s*200\s*,\s*"
            r"1000\s*\)"
            r"\s*;", 1, 1),
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
    ("palette.obj", 0x10A244): (
        SourceRule(
            "ftol keeps Dreamcast's const unsigned long magic local and "
            "retail-corroborated 0x59c00000 value",
            r"\A\s*const\s+unsigned\s+long\s+magic\s*=\s*"
            r"0x59c00000\s*;", 1, 1),
        SourceRule(
            "ftol keeps Dreamcast's double mutation before the low-word "
            "return; the union spelling is the gate-clean representation "
            "of the retail-corroborated type pun",
            r"TFloatLongBits\s+magic_value\s*;\s*"
            r"TDoubleLongBits\s+result\s*;\s*"
            r"result\s*\.\s*value\s*=\s*d\s*;\s*"
            r"magic_value\s*\.\s*bits\s*=\s*magic\s*;\s*"
            r"result\s*\.\s*value\s*\+=\s*"
            r"magic_value\s*\.\s*value\s*;\s*"
            r"return\s+result\s*\.\s*words\s*\[\s*0\s*\]\s*;",
            1, 1),
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
    ("palette.obj", 0x10B1EC): (
        SourceRule(
            "TPalette16::AdjustSaturation keeps Dreamcast's red, green and "
            "blue const normalization statements in source-row order",
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
            "TPalette16::AdjustSaturation keeps Dreamcast's entry-10 loop, "
            "r/g/b channel statements, h/s/v lifetime and RGBToHSV boundary",
            r"for\s*\(\s*int\s+i\s*=\s*10\s*;\s*i\s*<\s*256\s*;\s*"
            r"\+\+\s*i\s*\)\s*\{\s*"
            r"unsigned\s+int\s+r\s*=\s*\(\s*data\s*\[\s*i\s*\]\s*&"
            r"\s*red_mask\s*\)\s*\*\s*red_norm\s*;\s*"
            r"unsigned\s+int\s+g\s*=\s*\(\s*data\s*\[\s*i\s*\]\s*&"
            r"\s*green_mask\s*\)\s*\*\s*green_norm\s*;\s*"
            r"unsigned\s+int\s+b\s*=\s*\(\s*data\s*\[\s*i\s*\]\s*&"
            r"\s*blue_mask\s*\)\s*\*\s*blue_norm\s*;\s*"
            r"float\s+h\s*;\s*float\s+s\s*;\s*float\s+v\s*;\s*"
            r"RGBToHSV\s*\(\s*r\s*,\s*g\s*,\s*b\s*,\s*&\s*h\s*,\s*"
            r"&\s*s\s*,\s*&\s*v\s*\)\s*;", 1, 1),
        SourceRule(
            "TPalette16::AdjustSaturation keeps Dreamcast's <=1 multiply "
            "and >1 reciprocal arms before HSVToRGB and the packed write",
            r"if\s*\(\s*amount\s*<=\s*1\.0f\s*\)\s*\{\s*"
            r"s\s*\*=\s*amount\s*;\s*\}\s*else\s*\{\s*"
            r"s\s*=\s*1\.0f\s*-\s*\(\s*1\.0f\s*-\s*s\s*\)\s*/\s*"
            r"amount\s*;\s*\}\s*HSVToRGB\s*\(\s*h\s*,\s*s\s*,\s*v"
            r"\s*,\s*&\s*r\s*,\s*&\s*g\s*,\s*&\s*b\s*\)\s*;.*?"
            r"\(\s*\(\s*r\s*/\s*red_norm\s*\)\s*&\s*red_mask\s*\)"
            r"\s*\|\s*\(\s*\(\s*g\s*/\s*green_norm\s*\)\s*&\s*"
            r"green_mask\s*\)\s*\|\s*\(\s*\(\s*b\s*/\s*blue_norm"
            r"\s*\)\s*&\s*blue_mask\s*\)", 1, 1),
    ),
    ("palette.obj", 0x10B484): (
        SourceRule(
            "TPalette16::AdjustHSV keeps Dreamcast's three const unsigned "
            "normalization statements, entry-10 loop, r/g/b and h/s/v "
            "locals, and RGBToHSV helper boundary in source-row order",
            r"\A\s*const\s+unsigned\s+int\s+red_norm\s*=\s*"
            r"std\s*::\s*numeric_limits\s*<\s*int\s*>\s*::\s*max\s*"
            r"\(\s*\)\s*/\s*red_mask\s*;\s*"
            r"const\s+unsigned\s+int\s+green_norm\s*=\s*"
            r"std\s*::\s*numeric_limits\s*<\s*int\s*>\s*::\s*max\s*"
            r"\(\s*\)\s*/\s*green_mask\s*;\s*"
            r"const\s+unsigned\s+int\s+blue_norm\s*=\s*"
            r"std\s*::\s*numeric_limits\s*<\s*int\s*>\s*::\s*max\s*"
            r"\(\s*\)\s*/\s*blue_mask\s*;.*?"
            r"for\s*\(\s*int\s+i\s*=\s*10\s*;\s*i\s*<\s*256\s*;\s*"
            r"\+\+\s*i\s*\)\s*\{\s*"
            r"unsigned\s+int\s+r\s*=.*?;\s*"
            r"unsigned\s+int\s+g\s*=.*?;\s*"
            r"unsigned\s+int\s+b\s*=.*?;\s*"
            r"float\s+h\s*;\s*float\s+s\s*;\s*float\s+v\s*;\s*"
            r"RGBToHSV\s*\(\s*r\s*,\s*g\s*,\s*b\s*,\s*&\s*h\s*,\s*"
            r"&\s*s\s*,\s*&\s*v\s*\)\s*;", 1, 1),
        SourceRule(
            "TPalette16::AdjustHSV keeps Dreamcast's nested hue interpolation, "
            "shortest-path wrap choice, and final unit-interval correction",
            r"if\s*\(\s*hue_adjust\s*>=\s*0\.0f\s*\)\s*\{\s*"
            r"float\s+delta\s*=\s*hue\s*-\s*h\s*;\s*"
            r"h\s*\+=\s*delta\s*\*\s*hue_adjust\s*;\s*"
            r"if\s*\(\s*fabs\s*\(\s*delta\s*\)\s*>\s*0\.5\s*\)\s*"
            r"\{\s*if\s*\(\s*delta\s*>\s*0\.0\s*\)\s*\{\s*"
            r"h\s*\+=\s*1\.0f\s*-\s*hue_adjust\s*;\s*\}\s*else\s*"
            r"\{\s*h\s*\+=\s*hue_adjust\s*;\s*\}\s*"
            r"if\s*\(\s*h\s*>=\s*1\.0\s*\)\s*\{\s*"
            r"h\s*-=\s*1\.0\s*;", 1, 1),
        SourceRule(
            "TPalette16::AdjustHSV keeps Dreamcast's independent saturation "
            "then value guards, each with <=1 multiply and >1 reciprocal arms",
            r"if\s*\(\s*saturation_adjust\s*>=\s*0\.0f\s*\)\s*\{\s*"
            r"if\s*\(\s*saturation_adjust\s*<=\s*1\.0f\s*\)\s*\{\s*"
            r"s\s*\*=\s*saturation_adjust\s*;\s*\}\s*else\s*\{\s*"
            r"s\s*=\s*1\.0f\s*-\s*\(\s*1\.0f\s*-\s*s\s*\)\s*/\s*"
            r"saturation_adjust\s*;\s*\}\s*\}\s*"
            r"if\s*\(\s*value_adjust\s*>=\s*0\.0\s*\)\s*\{\s*"
            r"if\s*\(\s*value_adjust\s*<=\s*1\.0f\s*\)\s*\{\s*"
            r"v\s*\*=\s*value_adjust\s*;\s*\}\s*else\s*\{\s*"
            r"v\s*=\s*1\.0f\s*-\s*\(\s*1\.0f\s*-\s*v\s*\)\s*/\s*"
            r"value_adjust\s*;", 1, 1),
        SourceRule(
            "TPalette16::AdjustHSV keeps Dreamcast's HSVToRGB helper boundary "
            "before the single retail-corroborated packed palette write",
            r"HSVToRGB\s*\(\s*h\s*,\s*s\s*,\s*v\s*,\s*&\s*r\s*,\s*"
            r"&\s*g\s*,\s*&\s*b\s*\)\s*;\s*"
            r"data\s*\[\s*i\s*\]\s*=\s*static_cast\s*<\s*"
            r"unsigned\s+short\s*>\s*\(\s*"
            r"\(\s*\(\s*r\s*/\s*red_norm\s*\)\s*&\s*red_mask\s*\)"
            r"\s*\|\s*\(\s*\(\s*g\s*/\s*green_norm\s*\)\s*&\s*"
            r"green_mask\s*\)\s*\|\s*\(\s*\(\s*b\s*/\s*blue_norm"
            r"\s*\)\s*&\s*blue_mask\s*\)\s*\)\s*;", 1, 1),
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
    ("palette.obj", 0x10BFD4): (
        SourceRule(
            "TPalette24::AdjustHSV keeps Dreamcast's three channel "
            "normalization statements, entry-10 loop, r/g/b and h/s/v "
            "locals, and RGBToHSV helper boundary in source-row order",
            r"\A\s*const\s+unsigned\s+int\s+red_norm\s*=\s*"
            r"std\s*::\s*numeric_limits\s*<\s*int\s*>\s*::\s*max\s*"
            r"\(\s*\)\s*/\s*255\s*;\s*"
            r"const\s+unsigned\s+int\s+green_norm\s*=\s*"
            r"std\s*::\s*numeric_limits\s*<\s*int\s*>\s*::\s*max\s*"
            r"\(\s*\)\s*/\s*255\s*;\s*"
            r"const\s+unsigned\s+int\s+blue_norm\s*=\s*"
            r"std\s*::\s*numeric_limits\s*<\s*int\s*>\s*::\s*max\s*"
            r"\(\s*\)\s*/\s*255\s*;.*?"
            r"for\s*\(\s*int\s+i\s*=\s*10\s*;\s*i\s*<\s*256\s*;\s*"
            r"\+\+\s*i\s*\)\s*\{\s*"
            r"unsigned\s+int\s+r\s*=\s*colors\s*\.\s*data\s*"
            r"\[\s*i\s*\]\s*\[\s*0\s*\]\s*\*\s*red_norm\s*;\s*"
            r"unsigned\s+int\s+g\s*=\s*colors\s*\.\s*data\s*"
            r"\[\s*i\s*\]\s*\[\s*1\s*\]\s*\*\s*green_norm\s*;\s*"
            r"unsigned\s+int\s+b\s*=\s*colors\s*\.\s*data\s*"
            r"\[\s*i\s*\]\s*\[\s*2\s*\]\s*\*\s*blue_norm\s*;\s*"
            r"float\s+h\s*;\s*float\s+s\s*;\s*float\s+v\s*;\s*"
            r"RGBToHSV\s*\(\s*r\s*,\s*g\s*,\s*b\s*,\s*&\s*h\s*,\s*"
            r"&\s*s\s*,\s*&\s*v\s*\)\s*;", 1, 1),
        SourceRule(
            "TPalette24::AdjustHSV keeps Dreamcast's nested hue interpolation, "
            "shortest-path wrap choice, and unit-interval correction",
            r"if\s*\(\s*hue_adjust\s*>=\s*0\.0f\s*\)\s*\{\s*"
            r"float\s+delta\s*=\s*hue\s*-\s*h\s*;\s*"
            r"h\s*\+=\s*delta\s*\*\s*hue_adjust\s*;\s*"
            r"if\s*\(\s*fabs\s*\(\s*delta\s*\)\s*>\s*0\.5\s*\)\s*"
            r"\{\s*if\s*\(\s*delta\s*>\s*0\.0\s*\)\s*\{\s*"
            r"h\s*\+=\s*1\.0f\s*-\s*hue_adjust\s*;\s*\}\s*else\s*"
            r"\{\s*h\s*\+=\s*hue_adjust\s*;\s*\}\s*"
            r"if\s*\(\s*h\s*>=\s*1\.0\s*\)\s*\{\s*"
            r"h\s*-=\s*1\.0\s*;", 1, 1),
        SourceRule(
            "TPalette24::AdjustHSV keeps Dreamcast's value adjustment before "
            "saturation adjustment and its dark/high-value saturation arm",
            r"if\s*\(\s*value_adjust\s*>=\s*0\.0\s*\)\s*\{\s*"
            r"if\s*\(\s*value_adjust\s*<=\s*1\.0f\s*\)\s*\{\s*"
            r"v\s*\*=\s*value_adjust\s*;\s*\}\s*else\s*\{\s*"
            r"v\s*=\s*1\.0f\s*-\s*\(\s*1\.0f\s*-\s*v\s*\)\s*/\s*"
            r"value_adjust\s*;\s*\}\s*\}\s*"
            r"if\s*\(\s*saturation_adjust\s*>=\s*0\.0f\s*\)\s*\{\s*"
            r"if\s*\(\s*saturation_adjust\s*<=\s*1\.0f\s*\)\s*\{\s*"
            r"s\s*\*=\s*saturation_adjust\s*;\s*\}\s*else\s+if\s*"
            r"\(\s*v\s*>\s*0\.75\s*&&\s*s\s*<\s*0\.25\s*\)\s*"
            r"\{\s*s\s*=\s*\(\s*1\.0f\s*-\s*v\s*\)\s*\*\s*s\s*"
            r"\*\s*saturation_adjust\s*\*\s*4\.0f\s*;\s*\}\s*else\s*"
            r"\{\s*s\s*=\s*1\.0f\s*-\s*\(\s*1\.0f\s*-\s*s\s*\)"
            r"\s*/\s*saturation_adjust\s*;", 1, 1),
        SourceRule(
            "TPalette24::AdjustHSV keeps Dreamcast's HSVToRGB helper boundary "
            "before three ordered retail-corroborated channel writes",
            r"HSVToRGB\s*\(\s*h\s*,\s*s\s*,\s*v\s*,\s*&\s*r\s*,\s*"
            r"&\s*g\s*,\s*&\s*b\s*\)\s*;\s*"
            r"colors\s*\.\s*data\s*\[\s*i\s*\]\s*\[\s*0\s*\]\s*=\s*"
            r"static_cast\s*<\s*unsigned\s+char\s*>\s*"
            r"\(\s*r\s*/\s*red_norm\s*\)\s*;\s*"
            r"colors\s*\.\s*data\s*\[\s*i\s*\]\s*\[\s*1\s*\]\s*=\s*"
            r"static_cast\s*<\s*unsigned\s+char\s*>\s*"
            r"\(\s*g\s*/\s*green_norm\s*\)\s*;\s*"
            r"colors\s*\.\s*data\s*\[\s*i\s*\]\s*\[\s*2\s*\]\s*=\s*"
            r"static_cast\s*<\s*unsigned\s+char\s*>\s*"
            r"\(\s*b\s*/\s*blue_norm\s*\)\s*;", 1, 1),
    ),
    ("palette.obj", 0x10C370): (
        SourceRule(
            "RGBToHSV keeps the retail-corroborated static red_hue, "
            "Dreamcast's named const max, nested extrema, value write and "
            "single saturation ternary in source order",
            r"static\s+const\s+float\s+red_hue\s*=\s*0\.0f\s*;\s*"
            r"const\s+unsigned\s+int\s+max\s*=\s*"
            r"\(\s*r\s*>\s*g\s*\?\s*r\s*:\s*g\s*\)\s*>\s*b\s*\?\s*"
            r"\(\s*r\s*>\s*g\s*\?\s*r\s*:\s*g\s*\)\s*:\s*b\s*;\s*"
            r"const\s+unsigned\s+int\s+min\s*=\s*"
            r"\(\s*r\s*<\s*g\s*\?\s*r\s*:\s*g\s*\)\s*<\s*b\s*\?\s*"
            r"\(\s*r\s*<\s*g\s*\?\s*r\s*:\s*g\s*\)\s*:\s*b\s*;\s*"
            r"\*\s*v\s*=\s*static_cast\s*<\s*float\s*>\s*\(\s*max\s*\)"
            r"\s*/\s*std\s*::\s*numeric_limits\s*<\s*int\s*>\s*::\s*"
            r"max\s*\(\s*\)\s*;\s*"
            r"\*\s*s\s*=\s*max\s*\?\s*static_cast\s*<\s*float\s*>\s*"
            r"\(\s*max\s*-\s*min\s*\)\s*/\s*static_cast\s*<\s*float\s*>"
            r"\s*\(\s*max\s*\)\s*:\s*0\.0f\s*;", 1, 1),
        SourceRule(
            "RGBToHSV keeps Dreamcast's arithmetic chroma guard and rc, gc, "
            "bc normalized-complement statements before sector selection",
            r"if\s*\(\s*max\s*-\s*min\s*\)\s*\{\s*"
            r"float\s+rc\s*=\s*static_cast\s*<\s*float\s*>\s*"
            r"\(\s*max\s*-\s*r\s*\)\s*/\s*static_cast\s*<\s*float\s*>\s*"
            r"\(\s*max\s*-\s*min\s*\)\s*;\s*"
            r"float\s+gc\s*=\s*static_cast\s*<\s*float\s*>\s*"
            r"\(\s*max\s*-\s*g\s*\)\s*/\s*static_cast\s*<\s*float\s*>\s*"
            r"\(\s*max\s*-\s*min\s*\)\s*;\s*"
            r"float\s+bc\s*=\s*static_cast\s*<\s*float\s*>\s*"
            r"\(\s*max\s*-\s*b\s*\)\s*/\s*static_cast\s*<\s*float\s*>\s*"
            r"\(\s*max\s*-\s*min\s*\)\s*;", 1, 1),
        SourceRule(
            "RGBToHSV keeps Dreamcast's red, green, blue sector order, the "
            "retail-corroborated red_hue load, negative wrap and gray arm",
            r"if\s*\(\s*r\s*==\s*max\s*\)\s*\{\s*"
            r"\*\s*h\s*=\s*\(\s*bc\s*-\s*gc\s*\)\s*/\s*6\.0f\s*\+\s*"
            r"red_hue\s*;\s*\}\s*else\s+if\s*\(\s*g\s*==\s*max\s*\)"
            r"\s*\{\s*\*\s*h\s*=\s*\(\s*rc\s*-\s*bc\s*\)\s*/\s*"
            r"6\.0f\s*\+\s*1\.0f\s*/\s*3\.0f\s*;\s*\}\s*else\s*\{\s*"
            r"\*\s*h\s*=\s*\(\s*gc\s*-\s*rc\s*\)\s*/\s*6\.0f\s*\+\s*"
            r"2\.0f\s*/\s*3\.0f\s*;\s*\}.*?"
            r"if\s*\(\s*\*\s*h\s*<\s*0\.0f\s*\)\s*\{\s*"
            r"\*\s*h\s*\+=\s*1\.0f\s*;\s*\}.*?else\s*\{\s*"
            r"\*\s*h\s*=\s*0\.0f\s*;", 1, 1),
    ),
    ("palette.obj", 0x10C564): (
        SourceRule(
            "HSVToRGB keeps Dreamcast's chromatic scope, sole const f "
            "local, fmod helper boundary, value scaling and p/q/t "
            "statement order",
            r"\A\s*if\s*\(\s*s\s*!=\s*0\.0f\s*\)\s*\{\s*"
            r"const\s+float\s+f\s*=\s*static_cast\s*<\s*float\s*>"
            r"\s*\(\s*fmod\s*\(\s*h\s*\*\s*6\.0f\s*,\s*1\.0\s*"
            r"\)\s*\)\s*;\s*"
            r"v\s*\*=\s*static_cast\s*<\s*float\s*>\s*\(\s*"
            r"std\s*::\s*numeric_limits\s*<\s*int\s*>\s*::\s*max"
            r"\s*\(\s*\)\s*\)\s*;\s*"
            r"const\s+float\s+p\s*=\s*v\s*\*\s*\(\s*1\.0f\s*-"
            r"\s*s\s*\)\s*;\s*"
            r"const\s+float\s+q\s*=\s*v\s*\*\s*\(\s*1\.0f\s*-"
            r"\s*s\s*\*\s*f\s*\)\s*;\s*"
            r"const\s+float\s+t\s*=\s*v\s*\*\s*\(\s*1\.0f\s*-"
            r"\s*s\s*\*\s*\(\s*1\.0f\s*-\s*f\s*\)\s*\)\s*;",
            1, 1),
        SourceRule(
            "HSVToRGB keeps one Dreamcast switch containing the six "
            "retail-corroborated hue-sector channel mappings",
            r"switch\s*\(\s*static_cast\s*<\s*int\s*>\s*\(\s*h\s*"
            r"\*\s*6\.0f\s*\)\s*\)\s*\{\s*"
            r"case\s+HSV_RED_SECTOR\s*:\s*"
            r"\*\s*r\s*=\s*ftol\s*\(\s*v\s*\)\s*;\s*"
            r"\*\s*g\s*=\s*ftol\s*\(\s*t\s*\)\s*;\s*"
            r"\*\s*b\s*=\s*ftol\s*\(\s*p\s*\)\s*;\s*break\s*;\s*"
            r"case\s+HSV_YELLOW_SECTOR\s*:\s*"
            r"\*\s*r\s*=\s*ftol\s*\(\s*q\s*\)\s*;\s*"
            r"\*\s*g\s*=\s*ftol\s*\(\s*v\s*\)\s*;\s*"
            r"\*\s*b\s*=\s*ftol\s*\(\s*p\s*\)\s*;\s*break\s*;\s*"
            r"case\s+HSV_GREEN_SECTOR\s*:\s*"
            r"\*\s*r\s*=\s*ftol\s*\(\s*p\s*\)\s*;\s*"
            r"\*\s*g\s*=\s*ftol\s*\(\s*v\s*\)\s*;\s*"
            r"\*\s*b\s*=\s*ftol\s*\(\s*t\s*\)\s*;\s*break\s*;\s*"
            r"case\s+HSV_CYAN_SECTOR\s*:\s*"
            r"\*\s*r\s*=\s*ftol\s*\(\s*p\s*\)\s*;\s*"
            r"\*\s*g\s*=\s*ftol\s*\(\s*q\s*\)\s*;\s*"
            r"\*\s*b\s*=\s*ftol\s*\(\s*v\s*\)\s*;\s*break\s*;\s*"
            r"case\s+HSV_BLUE_SECTOR\s*:\s*"
            r"\*\s*r\s*=\s*ftol\s*\(\s*t\s*\)\s*;\s*"
            r"\*\s*g\s*=\s*ftol\s*\(\s*p\s*\)\s*;\s*"
            r"\*\s*b\s*=\s*ftol\s*\(\s*v\s*\)\s*;\s*break\s*;\s*"
            r"case\s+HSV_MAGENTA_SECTOR\s*:\s*"
            r"\*\s*r\s*=\s*ftol\s*\(\s*v\s*\)\s*;\s*"
            r"\*\s*g\s*=\s*ftol\s*\(\s*p\s*\)\s*;\s*"
            r"\*\s*b\s*=\s*ftol\s*\(\s*q\s*\)\s*;\s*break\s*;",
            1, 1),
        SourceRule(
            "HSVToRGB keeps all nineteen retail-corroborated expansions of "
            "Dreamcast's ftol helper boundary",
            r"\bftol\s*\(", 19, 19),
        SourceRule(
            "HSVToRGB keeps Dreamcast's grayscale scope as one "
            "right-associated write through the ftol helper",
            r"else\s*\{\s*\*\s*r\s*=\s*\*\s*g\s*=\s*\*\s*b\s*=\s*"
            r"ftol\s*\(\s*v\s*\*\s*static_cast\s*<\s*float\s*>"
            r"\s*\(\s*std\s*::\s*numeric_limits\s*<\s*int\s*>\s*::"
            r"\s*max\s*\(\s*\)\s*\)\s*\)\s*;\s*\}", 1, 1),
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
    ("army.obj", 0x46658): (
        SourceRule(
            "do_post_attack keeps all seven Dreamcast-proven "
            "TTextResource::operator[] statement boundaries",
            r"\(\s*\*\s*gpGeneralText\s*\)\s*\[", 7, 7),
        SourceRule(
            "do_post_attack may not flatten operator[] into GetText",
            r"\bgpGeneralText\s*->\s*GetText\s*\(", 0, 0),
        SourceRule(
            "do_post_attack keeps the four separate shared min statements",
            r"(?<![_\w])min\s*\(", 4, 4),
        SourceRule(
            "do_post_attack may not bypass the source min wrapper",
            r"\b_cpp_min\s*\(", 0, 0),
        SourceRule(
            "the Vampire arm keeps dead_vampires, missing_life, and the "
            "two ordered damage_recovered min statements",
            r"\blong\s+dead_vampires\s*=\s*0\s*;\s*"
            r"long\s+missing_life\s*=.*?;\s*"
            r"long\s+damage_recovered\s*=\s*min\s*\(\s*iDamage\s*,\s*"
            r"total_life\s*\)\s*;\s*"
            r"damage_recovered\s*=\s*min\s*\(\s*damage_recovered\s*,\s*"
            r"missing_life\s*\)\s*;"),
        SourceRule(
            "the Gorgon arm keeps its accumulator before Is, its Random "
            "loop in do_post_attack, and two ordered min statements",
            r"case\s+CREATURE_MIGHTY_GORGON\s*:\s*\{\s*"
            r"int\s+stares\s*=\s*0\s*;\s*"
            r"if\s*\(\s*target\s*->\s*Is\s*\([^)]*\)\s*\)\s*\{\s*"
            r"for\s*\(\s*long\s+i\s*=\s*0\s*;\s*i\s*<\s*numTroops\s*;"
            r"\s*i\+\+\s*\)\s*\{.*?"
            r"\bRandom\s*\(\s*1\s*,\s*100\s*\).*?\bstares\+\+\s*;"
            r".*?\blong\s+dead\s*=\s*min\s*\(\s*stares\s*,\s*"
            r"target\s*->\s*numTroops\s*\)\s*;\s*"
            r"dead\s*=\s*min\s*\(\s*dead\s*,\s*"
            r"\(\s*numTroops\s*\+\s*9\s*\)\s*/\s*10\s*\)\s*;"),
        SourceRule(
            "do_post_attack may not recreate the artificial Vampire or "
            "Gorgon caller-shrink helpers",
            r"\b(?:drain_amount|roll_death_stares)\s*\(", 0, 0),
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
    ("army.obj", 0x46FB0): (
        SourceRule(
            "do_attack(int) keeps Dreamcast's sole one-argument defender "
            "get_attack_direction(this) boundary",
            r"\barmyToAttack\s*->\s*get_attack_direction\s*\(\s*this\s*\)",
            1, 1),
    ),
    ("army.obj", 0x475EC): (
        SourceRule("CheckLuck calls SRandom", r"\bSRandom\s*\("),
        SourceRule("CheckLuck preserves the min wrapper", r"\bmin\s*\("),
        SourceRule("CheckLuck uses TTextResource::operator[]",
                   r"\(\s*\*\s*gpGeneralText\s*\)\s*\["),
    ),
    ("army.obj", 0x47C04): (
        SourceRule(
            "can_shoot keeps Dreamcast's owner-sensitive "
            "army::enemy_is_adjacent(excluded) boundary",
            r"(?<![\w:>])(?:this\s*->\s*)?enemy_is_adjacent\s*\(\s*"
            r"excluded\s*\)", 1, 1),
    ),
    ("army.obj", 0x47C74): (
        SourceRule(
            "enemy_is_adjacent keeps Dreamcast's army::Is(1) boundary",
            r"(?<![\w:>])(?:this\s*->\s*)?Is\s*\(\s*1u?\s*\)",
            1, 1),
        SourceRule(
            "enemy_is_adjacent keeps Dreamcast's sole "
            "get_second_grid_index boundary",
            r"\bget_second_grid_index\s*\(\s*\)", 1, 1),
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
    ("army.obj", 0x4A348): (
        SourceRule(
            "get_berserk_targets keeps Dreamcast's sole can_shoot(0) "
            "boundary",
            r"\bcan_shoot\s*\(\s*(?:0|NULL)\s*\)", 1, 1),
        SourceRule(
            "get_berserk_targets keeps Dreamcast's searchArray::get_hex "
            "boundary",
            r"\bgpSearchArray\s*->\s*get_hex\s*\(\s*"
            r"other\s*->\s*gridIndex\s*\)", 1, 1),
        SourceRule(
            "get_berserk_targets keeps Dreamcast's clear then push_back "
            "statement order",
            r"\barmies\s*\.\s*clear\s*\(\s*\)\s*;\s*\}?[\s\S]*?"
            r"\barmies\s*\.\s*push_back\s*\(\s*other\s*\)", 1, 1),
    ),
    ("army.obj", 0x4A480): (
        SourceRule(
            "GoBerserk keeps Dreamcast's sole can_shoot(0) boundary",
            r"\bcan_shoot\s*\(\s*(?:0|NULL)\s*\)", 1, 1),
        SourceRule(
            "GoBerserk keeps all three Dreamcast get_owning_side "
            "boundaries",
            r"\bget_owning_side\s*\(", 3, 3),
        SourceRule(
            "GoBerserk keeps Dreamcast's shoot-arm return before the "
            "separate melee statement",
            r"\bfield_53dc\s*\[\s*get_owning_side\s*\(\s*\)\s*\]"
            r"\s*=\s*1\s*;\s*return\s*;\s*\}\s*"
            r"gpCombatManager\s*->\s*berserk_attack\s*\("),
    ),
    ("army.obj", 0x4A7AC): (
        SourceRule(
            "attack_hex keeps Dreamcast's sole one-argument "
            "get_attack_direction(target) boundary",
            r"\bget_attack_direction\s*\(\s*target\s*\)", 1, 1),
    ),
    ("army.obj", 0x4B354): (
        SourceRule(
            "get_second_grid_index keeps Dreamcast's army::Is(1) "
            "boundary",
            r"(?<![\w:>])(?:this\s*->\s*)?Is\s*\(\s*1u?\s*\)",
            1, 1),
        SourceRule(
            "get_second_grid_index keeps Dreamcast's OffsetToFront(-1) "
            "boundary",
            r"\bOffsetToFront\s*\(\s*-\s*1\s*\)", 1, 1),
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
    ("mapcell.obj", 0xEDA4C): (
        SourceRule(
            "CObject::get_trigger keeps Dreamcast's result_x/result_y "
            "locals, reference-form FindTrigger call and type_point ctor",
            r"\A\s*int\s+result_x\s*;\s*int\s+result_y\s*;\s*"
            r"FindTrigger\s*\(\s*result_x\s*,\s*result_y\s*\)\s*;\s*"
            r"return\s+type_point\s*\(\s*result_x\s*,\s*result_y\s*,"
            r"\s*z\s*\)\s*;\s*\Z"),
    ),
    ("mapcell.obj", 0xF0DF4): (
        SourceRule(
            "readHeroData keeps Dreamcast's function-scope local roster "
            "and declaration order",
            r"\A\s*char\s+padding\s*\[\s*16\s*\]\s*;\s*"
            r"unsigned\s+char\s+isRandomHero\s*;\s*"
            r"(?:THeroID|int)\s+HeroID\s*;\s*"
            r"int\s+int_buffer\s*;\s*short\s+short_buffer\s*;\s*"
            r"char\s+Owner\s*;\s*char\s+customName\s*;\s*"
            r"int\s+count\s*;\s*int\s+experience\s*;\s*"
            r"int\s+x\s*;\s*char\s+char_buffer\s*;\s*"
            r"char\s+tempText\s*\[\s*100\s*\]\s*=\s*"
            r"\{\s*0\s*\}\s*;\s*HeroExtra\s*\*\s*"
            r"hero_data\s*;"),
        SourceRule(
            "readHeroData has one shared Dreamcast int_buffer local",
            r"\bint\s+int_buffer\s*;", minimum=1, maximum=1),
        SourceRule(
            "readHeroData has one shared Dreamcast short_buffer local",
            r"\bshort\s+short_buffer\s*;", minimum=1, maximum=1),
        SourceRule(
            "readHeroData has one shared Dreamcast char_buffer local",
            r"\bchar\s+char_buffer\s*;", minimum=1, maximum=1),
        SourceRule(
            "readHeroData keeps Complete's retail-proved zero-experience "
            "path into GetStartingHeroId",
            READ_HERO_COMPLETE_EXPERIENCE_RE),
        SourceRule(
            "readHeroData does not restore Dreamcast's retail-absent Random "
            "fallback",
            r"\bRandom\s*\(", minimum=0, maximum=0),
        SourceRule(
            "readHeroData keeps Complete's retail-proved custom-name group",
            READ_HERO_COMPLETE_NAME_RE),
        SourceRule(
            "readHeroData does not restore Dreamcast's retail-absent "
            "bounded name copy",
            r"\bstrncpy\s*\(", minimum=0, maximum=0),
        SourceRule(
            "readHeroData keeps Dreamcast's direct get_trigger assignment "
            "at the Complete tail",
            r"hero_data\s*->\s*location\s*=\s*heroObject\s*->\s*"
            r"get_trigger\s*\(\s*\)\s*;\s*return\s+0\s*;\s*\Z"),
        SourceRule(
            "readHeroData keeps Dreamcast's shared x local across the "
            "secondary-skill loop",
            r"\bint\s+x\s*;.*?for\s*\(\s*x\s*=\s*0\s*;\s*x\s*<\s*"
            r"hero_data\s*->\s*NumSecondarySkills\s*;\s*\+\+\s*x\s*\)"
            r"\s*\{.*?hero_data\s*->\s*secondarySkill\s*\[\s*x\s*\]"
            r"\s*=.*?hero_data\s*->\s*secondarySkillLevel\s*"
            r"\[\s*x\s*\]\s*="),
        SourceRule(
            "readHeroData keeps Dreamcast's shared x local across the army "
            "loop",
            r"for\s*\(\s*x\s*=\s*0\s*;\s*x\s*<\s*armyGroup\s*::\s*"
            r"ARMY_GROUP_SLOT_COUNT\s*;\s*\+\+\s*x\s*\)\s*\{.*?"
            r"hero_data\s*->\s*armies\s*\[\s*x\s*\]\s*=.*?"
            r"hero_data\s*->\s*numTroops\s*\[\s*x\s*\]\s*="),
        SourceRule(
            "readHeroData reuses Dreamcast's x local for both artifact "
            "loops",
            r"for\s*\(\s*x\s*=\s*0\s*;\s*x\s*<\s*count\s*;\s*"
            r"\+\+\s*x\s*\)\s*\{.*?hero_data\s*->\s*artifacts\s*"
            r"\[\s*x\s*\].*?for\s*\(\s*x\s*=\s*0\s*;\s*x\s*<\s*"
            r"hero_data\s*->\s*numInBackpack\s*;\s*\+\+\s*x\s*\)"
            r"\s*\{.*?hero_data\s*->\s*backpack\s*\[\s*x\s*\]"),
        SourceRule(
            "readHeroData does not move its recovered statement groups into "
            "invented caller-shrink helpers",
            r"\b(?:readHeroArmies|readHeroSecondarySkills)\s*\(",
            minimum=0, maximum=0),
    ),
    ("mapcell.obj", 0xEDAB8): (
        SourceRule(
            "CObject::FindTrigger keeps Dreamcast's reference assignments, "
            "ObjType/Vert/Horiz locals and nested trigger-mask scan",
            r"\A\s*result_x\s*=\s*-1\s*;\s*result_y\s*=\s*-1\s*;\s*"
            r"CObjectType\s*\*\s*ObjType\s*=\s*&\s*gpGame\s*->\s*"
            r"worldMap\s*\.\s*objectTypes\s*\[\s*typeIndex\s*\]\s*;"
            r"\s*for\s*\(\s*int\s+Vert\s*=\s*0\s*;\s*Vert\s*<\s*"
            r"ObjType\s*->\s*height\s*;\s*\+\+\s*Vert\s*\)\s*\{.*?"
            r"for\s*\(\s*int\s+Horiz\s*=\s*0\s*;\s*Horiz\s*<\s*"
            r"ObjType\s*->\s*width\s*;\s*\+\+\s*Horiz\s*\)\s*\{.*?"
            r"if\s*\(\s*ObjType\s*->\s*triggerCells\s*\[\s*47\s*-\s*"
            r"Vert\s*\*\s*8\s*-\s*Horiz\s*\]\s*\)\s*\{\s*"
            r"result_x\s*=\s*x\s*-\s*Horiz\s*;\s*result_y\s*=\s*y\s*"
            r"-\s*Vert\s*;\s*return\s*;\s*\}\s*\}\s*\}\s*\Z"),
    ),
    ("mapcell.obj", 0xEBF6C): (
        SourceRule(
            "TObjectCell::get_object keeps Dreamcast's vector subscript "
            "helper body instead of a flattened object-pool address",
            r"\A\s*return\s+&\s*gpGame\s*->\s*worldMap\s*\.\s*objects"
            r"\s*\[\s*objectIndex\s*\]\s*;\s*\Z"),
    ),
    ("mapcell.obj", 0xEBF98): (
        SourceRule(
            "NewmapCell::get_trigger_cell keeps Dreamcast's exact two-local "
            "shape and the CObject::get_trigger/game::get_cell boundaries",
            r"\A\s*if\s*\(\s*is_trigger\s*\)\s*return\s+this\s*;\s*"
            r"if\s*\(\s*type\s*==\s*NOTHING\s*\|\|\s*type\s*==\s*"
            r"ANCHOR_POINT\s*\|\|\s*type\s*==\s*EVENT\s*\|\|\s*type\s*"
            r"==\s*HOLY_GRAIL\s*\)\s*return\s+0\s*;\s*if\s*\(\s*"
            r"object_type_index\s*<\s*0\s*\|\|\s*object_type_index\s*"
            r">=\s*gpGame\s*->\s*worldMap\s*\.\s*objects\s*\.\s*size"
            r"\s*\(\s*\)\s*\)\s*return\s+0\s*;\s*CObject\s*\*\s*"
            r"object\s*=\s*&\s*gpGame\s*->\s*worldMap\s*\.\s*objects"
            r"\s*\[\s*object_type_index\s*\]\s*;\s*type_point\s+"
            r"location\s*=\s*object\s*->\s*get_trigger\s*\(\s*\)\s*;"
            r"\s*if\s*\(\s*location\s*\.\s*x\s*<\s*0\s*\)\s*"
            r"return\s+0\s*;\s*return\s+gpGame\s*->\s*get_cell\s*\("
            r"\s*location\s*\)\s*;\s*\Z"),
    ),
    ("mapcell.obj", 0xEC098): (
        SourceRule(
            "NewmapCell::get_map_object keeps Dreamcast's scoped hero and "
            "boat locals, GetHero/GetBoat calls and nested "
            "get_obscured_object helper boundaries",
            r"\A\s*if\s*\(\s*type\s*==\s*HERO\s*\)\s*\{\s*"
            r"hero\s*\*\s*current_hero\s*=\s*gpGame\s*->\s*GetHero"
            r"\s*\(\s*extraInfo\s*\)\s*;\s*return\s+current_hero\s*->\s*"
            r"get_obscured_object\s*\(\s*\)\s*;\s*\}\s*"
            r"if\s*\(\s*type\s*==\s*BOAT\s*\)\s*\{\s*"
            r"boat\s*\*\s*current_boat\s*=\s*gpGame\s*->\s*GetBoat"
            r"\s*\(\s*extraInfo\s*\)\s*;\s*return\s+current_boat\s*->\s*"
            r"get_obscured_object\s*\(\s*\)\s*;\s*\}\s*"
            r"return\s+type\s*;\s*\Z"),
    ),
    ("mapcell.obj", 0xEC254): (
        SourceRule(
            "NewmapCell::is_diggable keeps Dreamcast's terrain and "
            "passability guards as separate statements",
            r"\A\s*if\s*\(\s*GroundSet\s*==\s*eTerrainWater\s*\|\|\s*"
            r"GroundSet\s*==\s*eTerrainRock\s*\)\s*return\s+0\s*;\s*"
            r"if\s*\(\s*!\s*\(\s*flags_00_11\s*&\s*0x40\s*\)\s*\)"
            r"\s*return\s+0\s*;"),
        SourceRule(
            "NewmapCell::is_diggable keeps the object_type local and "
            "get_map_object boundary before the three shared type tests",
            r"TAdventureObjectType\s+object_type\s*=\s*get_map_object"
            r"\s*\(\s*\)\s*;\s*if\s*\(\s*object_type\s*!=\s*"
            r"ANCHOR_POINT\s*\)\s*\{\s*if\s*\(\s*object_type\s*!=\s*"
            r"HOLY_GRAIL\s*&&\s*object_type\s*!=\s*NOTHING\s*\)\s*"
            r"return\s+0\s*;\s*\}\s*else\s*\{"),
        SourceRule(
            "NewmapCell::is_diggable keeps Dreamcast's long i anchor loop "
            "and the TObjectCell::get_object/CObject::get_type chain",
            r"for\s*\(\s*long\s+i\s*=\s*0\s*;\s*i\s*<\s*objects\s*\.\s*"
            r"size\s*\(\s*\)\s*;\s*\+\+\s*i\s*\)\s*\{\s*if\s*\(\s*"
            r"objects\s*\[\s*i\s*\]\s*\.\s*get_object\s*\(\s*\)"
            r"\s*->\s*get_type\s*\(\s*\)\s*==\s*TERRAIN_HOLE\s*\)"
            r"\s*return\s+0\s*;\s*\}\s*\}\s*return\s+1\s*;\s*\Z"),
    ),
    ("mapcell.obj", 0xEC3B4): (
        SourceRule(
            "NewmapCell::get_special_terrain keeps Dreamcast's our_hero "
            "scope, both obscurer helpers, and the direct GARRISON arm "
            "before the reverse scan",
            r"\A\s*if\s*\(\s*type\s*==\s*HERO\s*&&\s*\(\s*cellFlags"
            r"\s*&\s*0x1000\s*\)\s*\)\s*\{\s*hero\s*\*\s*our_hero\s*"
            r"=\s*gpGame\s*->\s*GetHero\s*\(\s*extraInfo\s*\)\s*;\s*"
            r"if\s*\(\s*our_hero\s*->\s*get_obscured_object\s*\(\s*\)"
            r"\s*==\s*GARRISON\s*&&\s*our_hero\s*->\s*"
            r"obscured_is_trigger\s*\(\s*\)\s*&&\s*objectIndex\s*==\s*1"
            r"\s*\)\s*return\s+GARRISON\s*;\s*\}\s*"
            r"if\s*\(\s*type\s*==\s*GARRISON\s*&&\s*\(\s*cellFlags"
            r"\s*&\s*0x1000\s*\)\s*&&\s*objectIndex\s*==\s*1\s*\)"
            r"\s*return\s+type\s*;\s*for\s*\("),
        SourceRule(
            "NewmapCell::get_special_terrain keeps Dreamcast's signed "
            "post-decrement i loop and TObjectCell/CObject helper chain, "
            "plus Complete's retail-proved ten terrain answers in order",
            r"for\s*\(\s*int\s+i\s*=\s*objects\s*\.\s*size\s*\(\s*\)"
            r"\s*;\s*i\s*--\s*>\s*0\s*;\s*\)\s*\{\s*"
            r"CObject\s*\*\s*object\s*=\s*objects\s*\[\s*i\s*\]\s*\.\s*"
            r"get_object\s*\(\s*\)\s*;\s*CObjectType\s*\*\s*"
            r"object_type\s*=\s*object\s*->\s*get_object_type_ptr"
            r"\s*\(\s*\)\s*;\s*if\s*\(\s*object_type\s*->\s*objectType"
            r"\s*==\s*CURSED_GROUND\s*\|\|\s*object_type\s*->\s*objectType"
            r"\s*==\s*MAGIC_PLAINS\s*\|\|\s*object_type\s*->\s*objectType"
            r"\s*==\s*HOLY_GROUND\s*\|\|\s*object_type\s*->\s*objectType"
            r"\s*==\s*EVIL_FOG\s*\|\|\s*object_type\s*->\s*objectType"
            r"\s*==\s*CLOVER_FIELD_2\s*\|\|\s*object_type\s*->\s*objectType"
            r"\s*==\s*FAVORABLE_WINDS\s*\|\|\s*object_type\s*->\s*"
            r"objectType\s*==\s*LUCID_POOLS\s*\|\|\s*object_type\s*->\s*"
            r"objectType\s*==\s*FIERY_FIELDS\s*\|\|\s*object_type\s*->\s*"
            r"objectType\s*==\s*ROCKLANDS\s*\|\|\s*object_type\s*->\s*"
            r"objectType\s*==\s*MAGIC_CLOUDS\s*\)\s*return\s+object_type"
            r"\s*->\s*objectType\s*;\s*\}\s*return\s+NOTHING\s*;\s*\Z"),
    ),
    ("mapcell.obj", 0xF4A9C): (
        SourceRule(
            "type_obscuring_object::get_obscured_object keeps Dreamcast's "
            "guarded obscuredType return followed by the separate NOTHING "
            "return; flattening it changes nested Windows inlining",
            r"\A\s*if\s*\(\s*valid\s*\)\s*return\s+obscuredType\s*;\s*"
            r"return\s+NOTHING\s*;\s*\Z"),
    ),
    ("findpath.obj", 0xA113C): (
        SourceRule(
            "type_obscuring_object::obscured_is_trigger keeps Dreamcast's "
            "direct was_trigger byte accessor",
            r"\A\s*return\s+was_trigger\s*;\s*\Z"),
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
    ("singleselectionwindow.obj", 0x135DA8): (
        SourceRule(
            "ShowWidget keeps Dreamcast's sole pWidget local, GetWidget "
            "assignment, and null-return guard in recovered order",
            r"\A\s*widget\s*\*\s*pWidget\s*=\s*GetWidget\s*\(\s*id\s*\)"
            r"\s*;\s*if\s*\(\s*!\s*pWidget\s*\)\s*return\s*;"),
        SourceRule(
            "ShowWidget keeps Dreamcast's show then enable helper order, "
            "including the self IsHost and m_flag65 alternatives",
            r"\bpWidget\s*->\s*show\s*\(\s*\)\s*;\s*"
            r"pWidget\s*->\s*enable\s*\(\s*IsHost\s*\(\s*\)\s*\|\|\s*"
            r"m_flag65\s*\)\s*;"),
    ),
    ("singleselectionwindow.obj", 0x136388): (
        SourceRule(
            "SetupAdvancedOptions keeps Dreamcast's reset-loop i local "
            "inside the mapChanged scope",
            r"if\s*\(\s*mapChanged\s*\)\s*\{\s*for\s*\(\s*int\s+i\s*="
            r"\s*0\s*;\s*i\s*<\s*CNetPlayerHandler\s*::\s*MAX_PLAYERS"),
        SourceRule(
            "SetupAdvancedOptions keeps Dreamcast's function-scope main-loop "
            "i and nextColor in their raw NB11 symbol order",
            r"\bint\s+i\s*(?:=\s*0\s*)?;\s*int\s+nextColor\s*=\s*0\s*;.*?"
            r"\bUpdateGameVars\s*\(\s*\)\s*;\s*for\s*\(\s*(?:i\s*=\s*0)?\s*;"
            r"\s*i\s*<"
            r"\s*CNetPlayerHandler\s*::\s*MAX_PLAYERS"),
        SourceRule(
            "SetupAdvancedOptions keeps Dreamcast's strNbr local and "
            "compiled-out zero initializer inside the main loop",
            r"for\s*\(\s*i\s*=\s*0\s*;[^)]*\)\s*\{\s*int\s+strNbr\s*="
            r"\s*0\s*;", 1, 1),
        SourceRule(
            "SetupAdvancedOptions keeps Dreamcast's dedicated town/hero "
            "icon-local lexical scope",
            r"townButton\s*->\s*y\s*=.*?;\s*\{\s*widget\s*\*\s*"
            r"townIcon\s*=.*?widget\s*\*\s*heroIcon\s*=.*?widget\s*\*\s*"
            r"heroLeft\s*=.*?widget\s*\*\s*heroRight\s*=.*?"
            r"heroRight\s*->\s*y\s*=.*?;\s*\}\s*playerName\s*->\s*y\s*="),
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
    ("singleselectionwindow.obj", 0x1480CC): (
        SourceRule(
            "CNewPlayerUpdateProc::Go keeps Dreamcast's sole initMsg local "
            "and CGameHeaderInfoInitMsgEx constructor boundary with the "
            "version, HeadersA count, and network-mode arguments in order",
            r"\A\s*CGameHeaderInfoInitMsgEx\s+initMsg\s*\(\s*"
            r"gUnnamed69fbe8\s*->\s*gameVersion\s*,\s*"
            r"gUnnamed69fbe8\s*->\s*HeadersA\.size\s*\(\s*\)\s*,\s*"
            r"gUnnamed69fbe8\s*->\s*m_flag64\s*\)\s*;", 1, 1),
        SourceRule(
            "CNewPlayerUpdateProc::Go keeps Dreamcast's constructor then "
            "TransmitRemoteDataDPID statement order and retail's dpid/flag "
            "arguments",
            r"CGameHeaderInfoInitMsgEx\s+initMsg\s*\([^;]*\)\s*;\s*"
            r"TransmitRemoteDataDPID\s*\(\s*&\s*initMsg\s*,\s*m_dpid\s*,"
            r"\s*false\s*,\s*true\s*\)\s*;", 1, 1),
    ),
    ("singleselectionwindow.obj", 0x148130): (
        SourceRule(
            "CNewPlayerUpdateProc::Tick keeps Dreamcast's ElapsedSince "
            "throttle as its first statement",
            r"\A\s*if\s*\(\s*GameTime\s*::\s*ElapsedSince\s*\(\s*"
            r"m_lastSendTime\s*\)\s*<\s*75\s*\)\s*return\s*;", 1, 1),
        SourceRule(
            "CNewPlayerUpdateProc::Tick keeps the five-row i loop and its "
            "block-scoped CGameHeaderInfoMsg/CMapFileNameMsg alternatives "
            "over HeadersA in Dreamcast statement order",
            r"if\s*\(\s*m_nextHeader\s*<\s*gUnnamed69fbe8\s*->\s*"
            r"HeadersA\.size\s*\(\s*\)\s*\)\s*\{\s*for\s*\(\s*int\s+i"
            r"\s*=\s*0\s*;\s*i\s*<\s*5\s*;\s*\+\+\s*i\s*\)\s*\{.*?"
            r"if\s*\(\s*gUnnamed69fbe8\s*->\s*m_flag64\s*\)\s*\{.*?"
            r"CGameHeaderInfoMsg\s+msg\s*\(.*?HeadersA\s*\[\s*"
            r"m_nextHeader\s*\].*?RemoteFn_00512C80\s*\(\s*m_dpid\s*,"
            r"\s*1\s*,\s*1\s*\)\s*;\s*\}\s*else\s*\{.*?"
            r"CMapFileNameMsg\s+msg\s*\(.*?HeadersA\s*\[\s*"
            r"m_nextHeader\s*\].*?TransmitRemoteDataDPID\s*\(\s*&\s*"
            r"msg\s*,\s*m_dpid\s*,\s*true\s*,\s*false\s*\)\s*;",
            1, 1),
        SourceRule(
            "CNewPlayerUpdateProc::Tick keeps increment, HandleRequests, "
            "exhaustion test, and RequestConfirmation in recovered order",
            r"\+\+\s*m_nextHeader\s*;.*?m_requests\.size\s*\(\s*\).*?"
            r"HandleRequests\s*\(\s*\)\s*;.*?m_nextHeader\s*>=\s*"
            r"gUnnamed69fbe8\s*->\s*HeadersA\.size\s*\(\s*\).*?"
            r"RequestConfirmation\s*\(\s*\)\s*;\s*break\s*;", 1, 1),
        SourceRule(
            "CNewPlayerUpdateProc::Tick keeps the exhausted-list request "
            "drain before confirmation and refreshes m_lastSendTime last",
            r"else\s+if\s*\(.*?m_requests\.size\s*\(\s*\).*?\)\s*"
            r"\{\s*HandleRequests\s*\(\s*\)\s*;\s*"
            r"RequestConfirmation\s*\(\s*\)\s*;\s*\}\s*"
            r"m_lastSendTime\s*=\s*GameTime\s*::\s*Get\s*\(\s*\)\s*;",
            1, 1),
    ),
    ("singleselectionwindow.obj", 0x1484C8): (
        SourceRule(
            "CNewPlayerUpdateProc::Finish keeps Dreamcast's end, filter, "
            "scroll, and setup messages in recovered statement order",
            r"\A\s*CGameHeaderInfoEndMsg\s+endMsg\s*;\s*"
            r"TransmitRemoteDataDPID\s*\(\s*&\s*endMsg\s*,\s*m_dpid\s*,"
            r"\s*false\s*,\s*true\s*\)\s*;\s*"
            r"CSetFilterMsg\s+filterMsg\s*\(.*?\)\s*;\s*"
            r"TransmitRemoteDataDPID\s*\(\s*&\s*filterMsg\s*,\s*"
            r"m_dpid\s*,\s*false\s*,\s*true\s*\)\s*;\s*"
            r"CScrollMsg\s+scrollMsg\s*\(.*?\)\s*;\s*"
            r"TransmitRemoteDataDPID\s*\(\s*&\s*scrollMsg\s*,\s*"
            r"m_dpid\s*,\s*false\s*,\s*true\s*\)\s*;\s*"
            r"gUnnamed69fbe8\s*->\s*SendSetupInfo\s*\(\s*m_dpid\s*\)"
            r"\s*;", 1, 1),
        SourceRule(
            "CNewPlayerUpdateProc::Finish keeps Dreamcast's advanced and "
            "scenario pane click-message scopes before Complete's "
            "retail-proved filter pane extension",
            r"if\s*\(\s*gUnnamed69fbe8\s*->\s*inAdvancedOptions\s*\)"
            r"\s*\{\s*CClickMsg\s+clickMsg\s*\(\s*129\s*\)\s*;.*?"
            r"inScenarioOptions\s*\)\s*\{\s*CClickMsg\s+clickMsg\s*"
            r"\(\s*128\s*\)\s*;.*?inFilterOptions\s*\)\s*\{\s*"
            r"CClickMsg\s+clickMsg\s*\(\s*130\s*\)\s*;", 1, 1),
        SourceRule(
            "CNewPlayerUpdateProc::Finish keeps Dreamcast's final click, "
            "player-position send, face validation, and face broadcast "
            "in recovered order",
            r"CClickMsg\s+clickMsg\s*\(\s*gpGame\s*->\s*setup\."
            r"difficulty\s*\+\s*107\s*\)\s*;\s*"
            r"TransmitRemoteDataDPID\s*\(\s*&\s*clickMsg\s*,\s*m_dpid"
            r"\s*,\s*false\s*,\s*true\s*\)\s*;\s*"
            r"gUnnamed69fbe8\s*->\s*SendPlayerPositions\s*\(\s*m_dpid"
            r"\s*\)\s*;\s*gUnnamed69fbe8\s*->\s*CheckFaces\s*\(\s*\)"
            r"\s*;\s*gUnnamed69fbe8\s*->\s*SendPlayerFaces\s*\(\s*\)"
            r"\s*;", 1, 1),
    ),
    ("singleselectionwindow.obj", 0x14870C): (
        SourceRule(
            "CNewPlayerUpdateMan::NewPlayer keeps Dreamcast's sole index "
            "local and GetFirstAvailable boundary before the guarded scope",
            r"\A\s*int\s+index\s*=\s*GetFirstAvailable\s*\(\s*\)\s*;"
            r"\s*if\s*\(\s*index\s*!=\s*-\s*1\s*\)\s*\{", 1, 1),
        SourceRule(
            "CNewPlayerUpdateMan::NewPlayer keeps Dreamcast's proc "
            "construction then virtual Go statement order",
            r"m_procs\s*\[\s*index\s*\]\s*=\s*new\s+"
            r"CNewPlayerUpdateProc\s*\(\s*dpid\s*\)\s*;\s*"
            r"m_procs\s*\[\s*index\s*\]\s*->\s*Go\s*\(\s*\)\s*;",
            1, 1),
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
    ("singleselectionwindow.obj", 0x1425F0): (
        SourceRule(
            "SendPlayerFaces keeps Dreamcast's i/pPlayer/msg scopes and "
            "retail-proved occupied-human-seat bounds",
            r"\A\s*for\s*\(\s*int\s+i\s*=\s*1\s*;\s*i\s*<\s*8\s*;"
            r"\s*\+\+\s*i\s*\)\s*\{\s*CNetPlayerHandlerPlayer\s*\*"
            r"\s*pPlayer\s*=\s*&\s*m_players\.humanPlayers\s*\[\s*i"
            r"\s*\]\s*;\s*if\s*\(\s*pPlayer\s*->\s*IsHuman\s*\(\s*"
            r"\)\s*&&\s*pPlayer\s*->\s*playerPos\s*!=\s*-\s*1\s*\)"
            r"\s*\{\s*CRequestHeroFaceReplyMsg\s+msg\s*\(\s*"
            r"pPlayer\s*->\s*playerPos\s*,\s*pPlayer\s*->\s*heroIndex"
            r"\s*\)\s*;\s*TransmitRemoteDataDPID\s*\(\s*&\s*msg\s*,"
            r"\s*0\s*,\s*false\s*,\s*true\s*\)\s*;", 1, 1),
    ),
    ("singleselectionwindow.obj", 0x147ACC): (
        SourceRule(
            "CGameHeaderInfoEndMsg keeps its recovered empty derived body",
            r"\A\s*\Z", 1, 1),
    ),
    ("singleselectionwindow.obj", 0x147AF4): (
        SourceRule(
            "CNewSetupInfoMsg keeps its recovered CNetMsg boundary and "
            "setup assignment",
            r"\A\s*m_setup\s*=\s*\*\s*setup\s*;\s*\Z", 1, 1),
    ),
    ("singleselectionwindow.obj", 0x147BF0): (
        SourceRule(
            "CScrollMsg keeps its recovered map-then-index constructor "
            "statements",
            r"\A\s*m_map\s*=\s*map\s*;\s*m_index\s*=\s*index\s*;\s*"
            r"\Z", 1, 1),
    ),
    ("singleselectionwindow.obj", 0x147DA4): (
        SourceRule(
            "CSetFilterMsg keeps its recovered CNetMsg constructor and "
            "size assignment",
            r"\A\s*m_size\s*=\s*size\s*;\s*\Z", 1, 1),
    ),
    ("singleselectionwindow.obj", 0x147F28): (
        SourceRule(
            "CClickMsg keeps its recovered CNetMsg constructor and widget "
            "assignment",
            r"\A\s*m_widgetId\s*=\s*widgetId\s*;\s*\Z", 1, 1),
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
    ("swapmgr.obj", 0x15D150): (
        SourceRule(
            "handle_artifact_click keeps Complete's retail-relocation-proved "
            "hero::HeroFn_004E2840 replacement as the dragged-id/slot "
            "rejection predicate",
            r"if\s*\(\s*!\s*our_hero\s*->\s*HeroFn_004E2840\s*\(\s*"
            r"gHeroScreenDraggedArtifact\s*\.\s*artifactId\s*,\s*slot\s*"
            r"\)\s*\)\s*return\s*;",
            1, 1),
        SourceRule(
            "handle_artifact_click may not retain or duplicate Dreamcast's "
            "obsolete free artifactAllowedInSlot helper after Complete's "
            "relocation-proved member-call replacement",
            r"\bartifactAllowedInSlot\s*\(", 0, 0),
    ),
    ("swapmgr.obj", 0x15CDBC): (
        SourceRule(
            "update_all_slots keeps Dreamcast's nested two-hero, nineteen-"
            "slot UpdateSlot helper body",
            r"for\s*\(\s*int\s+iHero\s*=\s*0\s*;\s*"
            r"iHero\s*<\s*2\s*;\s*\+\+iHero\s*\)\s*"
            r"for\s*\(\s*int\s+slot\s*=\s*const_first_artifact_slot\s*;\s*"
            r"slot\s*<\s*kNumArtifactSlots\s*\+\s*1\s*;\s*"
            r"slot\+\+\s*\)\s*"
            r"UpdateSlot\s*\(\s*iHero\s*,\s*"
            r"static_cast\s*<\s*TArtifactSlot\s*>\s*\(\s*slot\s*\)\s*\)\s*;",
            1, 1),
    ),
    ("swapmgr.obj", 0x15EA00): (
        SourceRule(
            "Update keeps Dreamcast's message, primary-skill/widget refresh "
            "and trailing update_all_slots helper order",
            r"\bmessage\s+msg\s*;.*?"
            r"\bGetPrimarySkill\s*\(.*?"
            r"\bBroadcastMessage\s*\(.*?"
            r"\bupdate_all_slots\s*\(\s*\)\s*;\s*\Z",
            1, 1),
    ),
    ("swapmgr.obj", 0x15EC58): (
        SourceRule(
            "OnWidgetDeselect keeps Dreamcast's trade-done construction "
            "before GetOtherHero and transmission",
            r"CTradeRequestDoneMsg\s+requestDone\s*;\s*"
            r"hero\s*\*\s*otherHero\s*=\s*GetOtherHero\s*\(\s*\)\s*;\s*"
            r"TransmitRemoteData\s*\(\s*&\s*requestDone\s*,\s*"
            r"otherHero\s*->\s*owner\s*,\s*0\s*,\s*1\s*\)\s*;", 1, 1),
    ),
    ("swapmgr.obj", 0x15EF1C): (
        SourceRule(
            "OnReceiveFromAlly keeps Dreamcast's give-me-stuff construction "
            "before GetOtherHero and transmission",
            r"CGiveMeStuffMsg\s+giveMeStuff\s*;\s*"
            r"hero\s*\*\s*otherHero\s*=\s*GetOtherHero\s*\(\s*\)\s*;\s*"
            r"TransmitRemoteData\s*\(\s*&\s*giveMeStuff\s*,\s*"
            r"otherHero\s*->\s*owner\s*,\s*0\s*,\s*1\s*\)\s*;", 1, 1),
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
    # Complete directly revises the final pair: the cannot_attack expansion
    # inside retail consider_single_enchantment has the shared incapacity/Is
    # prefix followed by First Aid Tent and Ammo Cart, contradicting the old
    # Dreamcast Psychic/Magic Elemental comparisons.
    855: (SourceRule("cannot_attack keeps Complete's incapacity, mask, and both war-machine ids",
                     r"IsIncapacitated\s*\(\s*\)[^;]*Is\s*\(\s*1u\s*<<\s*21\s*\)[^;]*creatureType\s*==\s*ARMY_CREATURE_FIRST_AID_TENT[^;]*creatureType\s*==\s*ARMY_CREATURE_AMMO_CART"),),
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
    wall_target_pattern = (
        r"\bstruct\s+TWallTarget\s*\{\s*"
        r"short\s+target_hex\s*;\s*"
        r"short\s+blocked_row\s*;\s*"
        r"short\s+hit_x\s*;\s*"
        r"short\s+hit_y\s*;\s*"
        r"TWallSection\s+wall\s*;\s*"
        r"int\s+get_blocked_hex\s*\(\s*\)\s*const\s*\{\s*"
        r"if\s*\(\s*blocked_row\s*!=\s*-\s*1\s*\)\s*"
        r"return\s+gCastleWallColumns\s*\[\s*blocked_row\s*\]\s*;\s*"
        r"return\s+-\s*1\s*;\s*\}\s*\}\s*;\s*"
        r"static\s+const\s+TWallTarget\s+wallTargets\s*\[\s*8\s*\]\s*;"
    )
    defects: list[tuple[int, str]] = []
    if re.search(wall_target_pattern, masked, re.DOTALL) is None:
        token = re.search(r"\bTWallTarget\b", masked)
        line = text.count("\n", 0, token.start()) + 1 if token else 1
        defects.append((
            line,
            "cmbtmgr.h TWallTarget must retain Dreamcast's nested field "
            "names/types, get_blocked_hex body, and static wallTargets[8]",
        ))
    rules: tuple[tuple[str, str, str], ...] = (
        ("ValidHex", "cmbtmgr.h:1460 ValidHex must remain static bool with "
         "the attested two-bound predicate",
         r"\bstatic\s+bool\s+ValidHex\s*\(\s*int\s+(\w+)\s*\)\s*\{"
         r"\s*return\s+\1\s*>=\s*0\s*&&\s*\1\s*<\s*"
         r"COMBAT_GRID_CELLS\s*;\s*\}"),
        ("get_wall_strength", "cmbtmgr.h:1473 get_wall_strength must retain "
         "its typed wallTargets-to-wallStrength inline chain",
         r"\blong\s+get_wall_strength\s*\(\s*TWallTargetId\s+(\w+)\s*\)"
         r"\s*const\s*\{\s*return\s+wallStrength\s*\[\s*wallTargets\s*"
         r"\[\s*\1\s*\]\s*\.\s*wall\s*\]\s*;\s*\}"),
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


def game_assign_data_header_violations(text: str) -> list[tuple[int, str]]:
    """Audit game.h:311's source-visible NewSMapHeader helper body.

    Dreamcast CodeView supplies the four parameter identities and records the
    two adjacent ``std::string::operator=(const char*)`` calls.  Retail x86
    independently retains the outer AssignData expansion and the base-header
    assignment, although VC6 lowers the nested string operations through a
    mixture of retained and expanded STL calls.  The compiler-layer skew does
    not permit replacing the positive source operations with direct assign()
    calls merely because that can improve a local byte score.
    """
    masked = _source.mask(text)
    pattern = (
        r"\bvoid\s+AssignData\s*\(\s*CMapHeaderData\s*\*\s*pData\s*,\s*"
        r"char\s*\*\s*sName\s*,\s*char\s*\*\s*sDesc\s*\)\s*\{\s*"
        r"static_cast\s*<\s*CMapHeaderData\s*&\s*>\s*\(\s*\*\s*this"
        r"\s*\)\s*=\s*\*\s*pData\s*;\s*"
        r"mapName\s*=\s*sName\s*;\s*"
        r"mapDescription\s*=\s*sDesc\s*;\s*\}")
    if re.search(pattern, masked, re.DOTALL) is not None:
        return []
    token = re.search(r"\bAssignData\s*\(", masked)
    line = text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "Game.h:311 NewSMapHeader::AssignData must retain Dreamcast's "
             "pData/sName/sDesc parameters, base-header assignment, and "
             "ordered mapName/mapDescription operator= statements")]


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


def new_player_update_contract_violations(
        header_text: str, source_text: str) -> list[tuple[int, str]]:
    """Audit the DC-derived, retail-corroborated two-vtable type chain.

    Dreamcast proves the CNewPlayerUpdateProc construction/virtual-operation
    boundaries. Complete retail proves that common state is shared by two
    concrete three-slot vtables and, through the position of their vptr
    stores, that t_map_list_update is the derived override. The novtable task
    boundary is what keeps the directly-called non-virtual teardown exact.
    """
    header = _source.mask(header_text)
    source = _source.mask(source_text)
    task = (
        r"class\s+__declspec\s*\(\s*novtable\s*\)\s*"
        r"CNewPlayerUpdateTask\s*\{.*?"
        r"virtual\s+void\s+Go\s*\(\s*\)\s*=\s*0\s*;\s*"
        r"virtual\s+void\s+Tick\s*\(\s*\)\s*=\s*0\s*;\s*"
        r"virtual\s+void\s+Finish\s*\(\s*\)\s*=\s*0\s*;.*?"
        r"~\s*CNewPlayerUpdateTask\s*\(\s*\)\s*;.*?"
        r"unsigned\s+long\s+m_dpid\s*;.*?int\s+m_nextHeader\s*;.*?"
        r"vector\s*<\s*SHeaderRequest\s*>\s+m_requests\s*;.*?"
        r"unsigned\s+long\s+m_lastSendTime\s*;.*?"
        r"unsigned\s+char\s+m_finished\s*;")
    proc = (
        r"class\s+CNewPlayerUpdateProc\s*:\s*public\s+"
        r"CNewPlayerUpdateTask\s*\{.*?"
        r"CNewPlayerUpdateProc\s*\(\s*unsigned\s+long\s+dpid\s*\)"
        r"\s*\{\s*m_dpid\s*=\s*dpid\s*;\s*"
        r"m_nextHeader\s*=\s*0\s*;\s*m_finished\s*=\s*0\s*;\s*"
        r"m_lastSendTime\s*=\s*0\s*;\s*\}.*?"
        r"virtual\s+void\s+Go\s*\(\s*\)\s*;\s*"
        r"virtual\s+void\s+Tick\s*\(\s*\)\s*;\s*"
        r"virtual\s+void\s+Finish\s*\(\s*\)\s*;\s*"
        r"void\s+RequestConfirmation\s*\(\s*\)\s*;\s*"
        r"void\s+HandleRequests\s*\(\s*\)\s*;")
    derived = (
        r"class\s+t_map_list_update\s*:\s*public\s+"
        r"CNewPlayerUpdateProc\s*\{.*?"
        r"t_map_list_update\s*\(\s*unsigned\s+long\s+dpid\s*\)\s*;"
        r".*?virtual\s+void\s+Go\s*\(\s*\)\s*;\s*"
        r"virtual\s+void\s+Tick\s*\(\s*\)\s*;\s*"
        r"virtual\s+void\s+Finish\s*\(\s*\)\s*;")
    manager = (
        r"class\s+CNewPlayerUpdateMan\s*\{.*?"
        r"CNewPlayerUpdateTask\s*\*\s*m_procs\s*\[\s*8\s*\]\s*;")
    definitions = (
        r"VA\s*\(\s*0x00589240\s*,\s*0x2F\s*\).*?"
        r"t_map_list_update\s*::\s*t_map_list_update\s*"
        r"\(\s*unsigned\s+long\s+dpid\s*\)\s*:\s*"
        r"CNewPlayerUpdateProc\s*\(\s*dpid\s*\)\s*\{\s*\}",
        r"CNewPlayerUpdateTask\s*::\s*~\s*CNewPlayerUpdateTask\s*"
        r"\(\s*\)\s*\{\s*\}",
        r"inline\s+void\s+CNewPlayerUpdateProc\s*::\s*"
        r"RequestConfirmation\s*\(\s*\)\s*\{\s*"
        r"logFile\.Log\s*\(\s*DATA_COMPGEN\s*\(.*?"
        r"requestingConfirmLog.*?\)\s*\)\s*;\s*"
        r"CReqHeaderConfirmMsg\s+msg\s*;\s*"
        r"TransmitRemoteDataDPID\s*\(\s*&\s*msg\s*,\s*m_dpid\s*,\s*"
        r"false\s*,\s*true\s*\)\s*;\s*\}",
        r"VA\s*\(\s*0x00578010\s*,\s*0x272\s*\).*?"
        r"inline\s+void\s+CNewPlayerUpdateProc\s*::\s*HandleRequests\s*"
        r"\(\s*\)",
    )
    derived_body = re.search(
        r"class\s+t_map_list_update\s*:\s*public\s+"
        r"CNewPlayerUpdateProc\s*\{(.*?)\};", header, re.DOTALL)
    derived_owns_helper = (derived_body is not None and re.search(
        r"\bHandleRequests\s*\(", derived_body.group(1)) is not None)
    if (all(re.search(pattern, header, re.DOTALL) is not None
            for pattern in (task, proc, derived, manager))
            and all(re.search(pattern, source, re.DOTALL) is not None
                    for pattern in definitions)
            and not derived_owns_helper):
        return []
    token = re.search(r"\bCNewPlayerUpdateTask\b", header)
    line = header_text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "the NewPlayer update model must retain the novtable shared "
             "task, CNewPlayerUpdateProc field-initializing level, derived "
             "t_map_list_update override, interface-pointer manager, exact "
             "0x00589240 derived constructor boundary, shared teardown, and "
             "Dreamcast-owned base RequestConfirmation/HandleRequests "
             "boundaries (without an invented log vararg)")]


def game_header_info_init_contract_violations(
        header_text: str, source_text: str) -> list[tuple[int, str]]:
    """Audit the nested DC message constructors retained by exact retail Go.

    CodeView proves a CGameHeaderInfoInitMsg base constructor followed by a
    CGameHeaderInfoInitMsgEx constructor whose two body statements are memset
    and strncpy. Complete retail expands the full chain in
    CNewPlayerUpdateProc::Go and independently proves every field, argument,
    store, and send flag. The helper boundaries are source facts even though
    /Ob2 leaves no out-of-line x86 copy.
    """
    header = _source.mask(header_text)
    source = _source.mask(source_text)
    base = (
        r"class\s+CGameHeaderInfoInitMsg\s*:\s*public\s+CNetMsg\s*\{.*?"
        r"unsigned\s+long\s+m_numMaps\s*;.*?"
        r"unsigned\s+char\s+m_netGame\s*;.*?"
        r"CGameHeaderInfoInitMsg\s*\(\s*unsigned\s+long\s+numMaps\s*,"
        r"\s*unsigned\s+char\s+loadGameMode\s*,\s*unsigned\s+long\s+"
        r"msgSize\s*\)\s*:\s*CNetMsg\s*\(\s*RS_GAME_HEADER_INFO_INIT\s*,"
        r"\s*msgSize\s*\)\s*\{\s*m_numMaps\s*=\s*numMaps\s*;\s*"
        r"m_netGame\s*=\s*loadGameMode\s*;\s*\}\s*\};")
    derived = (
        r"class\s+CGameHeaderInfoInitMsgEx\s*:\s*public\s+"
        r"CGameHeaderInfoInitMsg\s*\{.*?char\s+m_version\s*\[\s*20\s*\]"
        r"\s*;.*?CGameHeaderInfoInitMsgEx\s*\(\s*const\s+char\s*\*\s*"
        r"version\s*,\s*unsigned\s+long\s+numMaps\s*,\s*unsigned\s+char"
        r"\s+loadGameMode\s*\)\s*:\s*CGameHeaderInfoInitMsg\s*\(\s*"
        r"numMaps\s*,\s*loadGameMode\s*,\s*sizeof\s*\(\s*"
        r"CGameHeaderInfoInitMsgEx\s*\)\s*\)\s*\{\s*"
        r"memset\s*\(\s*m_version\s*,\s*0\s*,\s*sizeof\s*\(\s*"
        r"m_version\s*\)\s*\)\s*;\s*strncpy\s*\(\s*m_version\s*,\s*"
        r"version\s*,\s*sizeof\s*\(\s*m_version\s*\)\s*-\s*1\s*\)"
        r"\s*;\s*\}\s*\};")
    go = (
        r"VA\s*\(\s*0x005789F0\s*,\s*0x9E\s*\).*?"
        r"CNewPlayerUpdateProc\s*::\s*Go\s*\(\s*\)\s*\{\s*"
        r"CGameHeaderInfoInitMsgEx\s+initMsg\s*\(\s*"
        r"gUnnamed69fbe8\s*->\s*gameVersion\s*,\s*"
        r"gUnnamed69fbe8\s*->\s*HeadersA\.size\s*\(\s*\)\s*,\s*"
        r"gUnnamed69fbe8\s*->\s*m_flag64\s*\)\s*;\s*"
        r"TransmitRemoteDataDPID\s*\(\s*&\s*initMsg\s*,\s*m_dpid\s*,"
        r"\s*false\s*,\s*true\s*\)\s*;\s*\}")
    if (re.search(base, header, re.DOTALL) is not None
            and re.search(derived, header, re.DOTALL) is not None
            and re.search(go, source, re.DOTALL) is not None):
        return []
    token = re.search(r"\bCGameHeaderInfoInitMsg\b", header)
    line = header_text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "the 1024 opener must retain Dreamcast's nested "
             "CGameHeaderInfoInitMsg/CGameHeaderInfoInitMsgEx constructor "
             "boundaries, ordered count/mode and memset/strncpy statements, "
             "and exact 0x005789f0 constructor-then-send body")]


def map_file_name_message_contract_violations(
        header_text: str) -> list[tuple[int, str]]:
    """Audit the DC constructor shape corroborated by both retail Tick arms."""
    header = _source.mask(header_text)
    shape = (
        r"class\s+CMapFileNameMsg\s*:\s*public\s+CNetMsg\s*\{.*?"
        r"unsigned\s+char\s+m_flag\s*;.*?int\s+m_number\s*;.*?"
        r"char\s+m_fileName\s*\[\s*0x40\s*\]\s*;.*?"
        r"int\s+m_townTypes\s*\[\s*8\s*\]\s*;.*?"
        r"FILETIME\s+m_fileTime\s*;.*?"
        r"CMapFileNameMsg\s*\(\s*unsigned\s+char\s+flag\s*,\s*int\s+"
        r"number\s*,\s*const\s+char\s*\*\s*fileName\s*,\s*int\s*\*\s*"
        r"townTypes\s*,\s*FILETIME\s+fileTime\s*\)\s*:\s*"
        r"CNetMsg\s*\(\s*RS_MAP_FILE_NAME\s*,\s*sizeof\s*\(\s*"
        r"CMapFileNameMsg\s*\)\s*\)\s*\{\s*"
        r"m_flag\s*=\s*flag\s*;\s*m_number\s*=\s*number\s*;\s*"
        r"strncpy\s*\(\s*m_fileName\s*,\s*fileName\s*,\s*0x3c\s*\)"
        r"\s*;\s*m_fileTime\s*=\s*fileTime\s*;\s*"
        r"memcpy\s*\(\s*m_townTypes\s*,\s*townTypes\s*,\s*sizeof\s*"
        r"\(\s*m_townTypes\s*\)\s*\)\s*;\s*\}\s*\};")
    if re.search(shape, header, re.DOTALL) is not None:
        return []
    token = re.search(r"\bCMapFileNameMsg\b", header)
    line = header_text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "CMapFileNameMsg must retain Dreamcast's explicit number, "
             "fileName, townTypes and by-value FILETIME constructor inputs "
             "and ordered number/strncpy/FILETIME/memcpy statements, with "
             "Complete's independently proved leading transfer flag")]


def finish_message_constructor_contract_violations(
        source_text: str) -> list[tuple[int, str]]:
    """Keep Finish's recovered out-of-class message constructor heads.

    Function-body rules preserve the ordered statements, but a constructor's
    base initializer lies outside those braces. Audit it directly so moving
    these definitions back into a flattened header view or dropping CNetMsg
    cannot hide in the historical generic-call backlog.
    """
    source = _source.mask(source_text)
    patterns = (
        r"CGameHeaderInfoEndMsg\s*::\s*"
        r"CGameHeaderInfoEndMsg\s*\(\s*\)\s*:\s*CNetMsg\s*\(\s*"
        r"RS_GAME_HEADER_INFO_END\s*,\s*sizeof\s*\(\s*"
        r"CGameHeaderInfoEndMsg\s*\)\s*\)\s*\{\s*\}",
        r"CNewSetupInfoMsg\s*::\s*CNewSetupInfoMsg\s*\(\s*"
        r"SGameSetupOptions\s*\*\s*setup\s*\)\s*:\s*CNetMsg\s*\(\s*"
        r"RS_NEW_SETUP_INFO\s*,\s*sizeof\s*\(\s*CNewSetupInfoMsg\s*\)"
        r"\s*\)\s*\{\s*m_setup\s*=\s*\*\s*setup\s*;\s*\}",
        r"CScrollMsg\s*::\s*CScrollMsg\s*\(\s*int\s+map\s*,"
        r"\s*int\s+index\s*\)\s*:\s*CNetMsg\s*\(\s*RS_SCROLL\s*,\s*"
        r"sizeof\s*\(\s*CScrollMsg\s*\)\s*\)\s*\{\s*m_map\s*=\s*"
        r"map\s*;\s*m_index\s*=\s*index\s*;\s*\}",
        r"CSetFilterMsg\s*::\s*CSetFilterMsg\s*\(\s*int\s+"
        r"size\s*\)\s*:\s*CNetMsg\s*\(\s*RS_SET_FILTER\s*,\s*sizeof"
        r"\s*\(\s*CSetFilterMsg\s*\)\s*\)\s*\{\s*m_size\s*=\s*size"
        r"\s*;\s*\}",
        r"CClickMsg\s*::\s*CClickMsg\s*\(\s*int\s+widgetId"
        r"\s*\)\s*:\s*CNetMsg\s*\(\s*RS_CLICK\s*,\s*sizeof\s*\(\s*"
        r"CClickMsg\s*\)\s*\)\s*\{\s*m_widgetId\s*=\s*widgetId\s*;"
        r"\s*\}",
    )
    if all(re.search(pattern, source, re.DOTALL) is not None
           for pattern in patterns):
        return []
    token = re.search(r"\bCGameHeaderInfoEndMsg\s*::", source)
    line = source_text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "Finish message constructors must remain out-of-class .cpp "
             "boundaries with their recovered CNetMsg initializers and "
             "ordered derived statements")]


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


def cobject_trigger_header_violations(text: str) \
        -> list[tuple[int, str]]:
    """Audit the published const/reference CObject trigger interfaces."""
    active = _source.mask(text)
    pattern = (
        r"\btype_point\s+get_trigger\s*\(\s*\)\s*const\s*;\s*"
        r"void\s+FindTrigger\s*\(\s*int\s*&\s*resultX\s*,\s*"
        r"int\s*&\s*resultY\s*\)\s*const\s*;")
    if re.search(pattern, active, re.DOTALL) is not None:
        return []
    token = re.search(r"\b(?:get_trigger|FindTrigger)\b", active)
    line = text.count("\n", 0, token.start()) + 1 if token else 1
    return [(line,
             "CObject must retain Dreamcast's get_trigger() const followed "
             "by const reference-form FindTrigger(int&, int&)")]


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
    if va is None:
        return body
    active = _source.mask(body)
    canonical = body
    if va in exact_vas:
        for spelling in PROVEN_CALL_SPELLINGS.get(key, ()):
            if va != spelling.caller_va \
                    or re.search(spelling.retail_pattern, active) is None:
                continue
            canonical = re.sub(
                spelling.retail_pattern, spelling.canonical_name, canonical)
    for spelling in NONEXACT_RETAIL_PROVEN_CALL_SPELLINGS.get(key, ()):
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


def retail_proven_dc_only_order_helpers(
        key: tuple[str, int], body: str, va: int | None) \
        -> tuple[frozenset[str], tuple[str, ...]]:
    """Return helper-order facts contradicted by a bounded retail body."""
    if va is None:
        return frozenset(), ()
    active = _source.mask(body)
    helpers: set[str] = set()
    descriptions: list[str] = []
    for skew in RETAIL_BYTE_PROVEN_ORDER_SKEWS.get(key, ()):
        if va != skew.caller_va \
                or re.search(skew.retail_pattern, active, re.DOTALL) is None:
            continue
        helpers.update(skew.dc_only_helpers)
        descriptions.append(skew.description)
    return frozenset(helpers), tuple(descriptions)


def proven_dc_only_removed_helpers(
        key: tuple[str, int], body: str, va: int | None,
        exact_vas: set[int]) -> tuple[frozenset[str], tuple[str, ...]]:
    """Return calls proved absent from one exact Complete revision body."""
    if va is None or va not in exact_vas:
        return frozenset(), ()
    active = _source.mask(body)
    helpers: set[str] = set()
    descriptions: list[str] = []
    for removal in PROVEN_REVISION_REMOVALS.get(key, ()):
        if va != removal.caller_va \
                or re.search(
                    removal.retail_pattern, active, re.DOTALL) is None:
            continue
        helpers.update(removal.dc_only_helpers)
        descriptions.append(removal.description)
    return frozenset(helpers), tuple(descriptions)


def retail_proven_dc_only_removed_helpers(
        key: tuple[str, int], body: str, va: int | None) \
        -> tuple[frozenset[str], tuple[str, ...]]:
    """Return DC calls contradicted by one bounded decoded retail body."""
    active = _source.mask(body)
    helpers: set[str] = set()
    descriptions: list[str] = []
    for removal in RETAIL_BYTE_PROVEN_REVISION_REMOVALS.get(key, ()):
        if (va is None and not removal.unclaimed_inline) \
                or (va is not None and va != removal.caller_va) \
                or re.search(
                    removal.retail_pattern, active, re.DOTALL) is None:
            continue
        helpers.update(removal.dc_only_helpers)
        descriptions.append(removal.description)
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

    # Dreamcast line 4395 proves attack_hex uses the one-argument header
    # wrapper. Complete retail then proves that wrapper's two-argument callee
    # expands into attack_hex's exact 41-block body. Losing this qualifier
    # preserves the source call but silently de-inlines its recovered shape,
    # so the call-only xref ratchet is insufficient on its own.
    direction_decl = re.compile(
        r"(?m)^[ \t]*(?P<inline>inline[ \t]+)?long[ \t]+"
        r"get_attack_direction\s*\(\s*long\s+our_hex\s*,\s*"
        r"const\s+army\s*\*\s*enemy\s*\)\s*const\s*;")
    direction_matches = list(direction_decl.finditer(masked))
    broken_direction = next(
        (match for match in direction_matches
         if match.group("inline") is None), None)
    if not direction_matches or broken_direction is not None:
        position = broken_direction.start() if broken_direction else 0
        defects.append((text.count("\n", 0, position) + 1,
                        "two-argument get_attack_direction must retain its "
                        "retail-proved inline contract"))

    # Dreamcast Army.h:810/815 supplies both class-body GetName wrappers and
    # CreatureType.h:296 supplies the nested GetArmyName wrapper. Their helper
    # boundaries are source facts even when VC6's shared /Ob2 budget chooses a
    # different expansion in one caller.
    get_name_decl = re.compile(
        r"(?m)^[ \t]*(?P<inline>inline[ \t]+)?const[ \t]+char[ \t]*\*"
        r"[ \t]*GetName\s*\(\s*(?:int\s+count\s*)?\)\s*const\s*;")
    get_name_matches = list(get_name_decl.finditer(masked))
    broken_get_name = next(
        (match for match in get_name_matches
         if match.group("inline") is None), None)
    if len(get_name_matches) != 2 or broken_get_name is not None:
        position = broken_get_name.start() if broken_get_name else 0
        defects.append((text.count("\n", 0, position) + 1,
                        "both GetName wrappers must retain their "
                        "Dreamcast-proven inline contract"))

    get_army_name_decl = re.compile(
        r"(?m)^[ \t]*(?P<inline>inline[ \t]+)?const[ \t]+char[ \t]*\*"
        r"[ \t]*GetArmyName\s*\(\s*int\s+type\s*,\s*int\s+count\s*\)"
        r"\s*;")
    get_army_name_matches = list(get_army_name_decl.finditer(masked))
    broken_get_army_name = next(
        (match for match in get_army_name_matches
         if match.group("inline") is None), None)
    if (len(get_army_name_matches) != 1
            or broken_get_army_name is not None):
        position = (broken_get_army_name.start()
                    if broken_get_army_name else 0)
        defects.append((text.count("\n", 0, position) + 1,
                        "GetArmyName must retain its Dreamcast-proven "
                        "inline contract"))
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
    redeclaration_probe = """\
void wanted() { KeepHelper(); }
VA(0x00401230, 0x10)
void wanted();
void following() { WrongHelper(); }
"""
    redeclaration_masked = _source.mask(redeclaration_probe)
    redeclaration_span = _body_for_claim(
        redeclaration_masked, _line_starts(redeclaration_probe), 2,
        "wanted")
    if redeclaration_span is None:
        failures.append("RVA-ordered redeclaration lost canonical definition")
    else:
        redeclaration_body = redeclaration_probe[
            redeclaration_span[0] + 1:redeclaration_span[1]]
        if "KeepHelper" not in redeclaration_body \
                or "WrongHelper" in redeclaration_body:
            failures.append(
                "RVA-ordered redeclaration audited the following function")
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
    show_widget_key = ("singleselectionwindow.obj", 0x135DA8)
    show_widget_probe = """\
widget* pWidget = GetWidget(id);
if (!pWidget)
    return;
pWidget->show();
pWidget->enable(IsHost() || m_flag65);
"""
    if contract_violations(show_widget_probe, show_widget_key):
        failures.append("aligned ShowWidget source shape did not pass")
    if missing_from_body(
            show_widget_probe.replace("IsHost()", "pDPlay->IsHost()"),
            host_edge, show_widget_key) != [
                ("TSingleSelectionWindow::IsHost", "IsHost")]:
        failures.append("ShowWidget peer receiver hid proven self-call IsHost")
    show_widget_mutations = (
        (show_widget_probe.replace(
            "widget* pWidget = GetWidget(id);",
            "widget* pWidget;\npWidget = GetWidget(id);"),
         "sole pWidget local"),
        (show_widget_probe.replace("if (!pWidget)\n    return;\n", ""),
         "null-return guard"),
        (show_widget_probe.replace(
            "pWidget->show();",
            "pWidget->send_message(widget::WIDGET_SET_STATUS, 6);"),
         "show then enable helper order"),
        (show_widget_probe.replace("IsHost() || ", ""),
         "self IsHost and m_flag65 alternatives"),
        (show_widget_probe.replace(" || m_flag65", ""),
         "self IsHost and m_flag65 alternatives"),
    )
    for probe, description in show_widget_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, show_widget_key)):
            failures.append("broken ShowWidget " + description
                            + " source shape passed")
    setup_probe = """\
if (mapChanged) {
    for (int i = 0; i < CNetPlayerHandler::MAX_PLAYERS; ++i) {}
}
int i;
int nextColor = 0;
widget* playerName;
UpdateGameVars();
for (i = 0; i < CNetPlayerHandler::MAX_PLAYERS; ++i) {
    int strNbr = 0;
    townButton->y = nextColor * 50 - y + 130;
    {
        widget* townIcon = GetWidget(i + 231);
        widget* heroIcon = GetWidget(i + 247);
        widget* heroLeft = GetWidget(i + 239);
        widget* heroRight = GetWidget(i + 255);
        heroRight->y = nextColor * 50 + 133;
    }
    playerName->y = nextColor * 50 + 151;
}
"""
    if contract_violations(setup_probe, host_key):
        failures.append("aligned SetupAdvancedOptions local scopes did not pass")
    setup_mutations = (
        (setup_probe.replace("for (int i = 0;", "for (i = 0;", 1),
         "reset-loop i"),
        (setup_probe.replace("int i;\nint nextColor = 0;",
                             "int nextColor = 0;\nint i;"),
         "function-scope main-loop"),
        (setup_probe.replace("int strNbr = 0;", "int strNbr;"),
         "strNbr"),
        (setup_probe.replace("    {\n        widget* townIcon",
                             "    widget* townIcon", 1)
                    .replace("\n    }\n    playerName->y",
                             "\n    playerName->y", 1),
         "icon-local lexical scope"),
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
    trade_done_key = ("swapmgr.obj", 0x15EC58)
    trade_done_probe = """\
CTradeRequestDoneMsg requestDone;
hero* otherHero = GetOtherHero();
TransmitRemoteData(&requestDone, otherHero->owner, 0, 1);
"""
    if contract_violations(trade_done_probe, trade_done_key):
        failures.append("aligned trade-done constructor order did not pass")
    if not contract_violations(
            trade_done_probe.replace(
                "CTradeRequestDoneMsg requestDone;\n"
                "hero* otherHero = GetOtherHero();",
                "hero* otherHero = GetOtherHero();\n"
                "CTradeRequestDoneMsg requestDone;"),
            trade_done_key):
        failures.append("reversed trade-done constructor order passed")
    give_stuff_key = ("swapmgr.obj", 0x15EF1C)
    give_stuff_probe = """\
CGiveMeStuffMsg giveMeStuff;
hero* otherHero = GetOtherHero();
TransmitRemoteData(&giveMeStuff, otherHero->owner, 0, 1);
"""
    if contract_violations(give_stuff_probe, give_stuff_key):
        failures.append("aligned give-me-stuff constructor order did not pass")
    if not contract_violations(
            give_stuff_probe.replace(
                "CGiveMeStuffMsg giveMeStuff;\n"
                "hero* otherHero = GetOtherHero();",
                "hero* otherHero = GetOtherHero();\n"
                "CGiveMeStuffMsg giveMeStuff;"),
            give_stuff_key):
        failures.append("reversed give-me-stuff constructor order passed")
    palette_ftol_key = ("palette.obj", 0x10A244)
    palette_ftol_probe = """\
const unsigned long magic = 0x59c00000;
TFloatLongBits magic_value;
TDoubleLongBits result;
result.value = d;
magic_value.bits = magic;
result.value += magic_value.value;
return result.words[0];
"""
    if contract_violations(palette_ftol_probe, palette_ftol_key):
        failures.append("aligned ftol source shape did not pass")
    palette_ftol_mutations = (
        (palette_ftol_probe.replace("const unsigned long magic",
                                    "unsigned long magic"),
         "const unsigned long magic local"),
        (palette_ftol_probe.replace("0x59c00000", "0x59b00000"),
         "0x59c00000 value"),
        (palette_ftol_probe.replace("result.value = d;\n", ""),
         "double mutation before the low-word return"),
        (palette_ftol_probe.replace("return result.words[0];",
                                    "return static_cast<long>(d);"),
         "low-word return"),
    )
    for probe, description in palette_ftol_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, palette_ftol_key)):
            failures.append("broken ftol " + description
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
    palette_saturation_key = ("palette.obj", 0x10B1EC)
    palette_saturation_probe = """\
const unsigned int red_norm =
    std::numeric_limits<int>::max() / red_mask;
const unsigned int green_norm =
    std::numeric_limits<int>::max() / green_mask;
const unsigned int blue_norm =
    std::numeric_limits<int>::max() / blue_mask;
for (int i = 10; i < 256; ++i) {
    unsigned int r = (data[i] & red_mask) * red_norm;
    unsigned int g = (data[i] & green_mask) * green_norm;
    unsigned int b = (data[i] & blue_mask) * blue_norm;
    float h;
    float s;
    float v;
    RGBToHSV(r, g, b, &h, &s, &v);
    if (amount <= 1.0f) {
        s *= amount;
    } else {
        s = 1.0f - (1.0f - s) / amount;
    }
    HSVToRGB(h, s, v, &r, &g, &b);
    data[i] = static_cast<unsigned short>(
        ((r / red_norm) & red_mask) |
        ((g / green_norm) & green_mask) |
        ((b / blue_norm) & blue_mask));
}
"""
    if contract_violations(palette_saturation_probe,
                           palette_saturation_key):
        failures.append(
            "aligned TPalette16::AdjustSaturation source shape did not pass")
    palette_saturation_mutations = (
        (palette_saturation_probe.replace(
            "const unsigned int red_norm =\n"
            "    std::numeric_limits<int>::max() / red_mask;\n"
            "const unsigned int green_norm =\n"
            "    std::numeric_limits<int>::max() / green_mask;",
            "const unsigned int green_norm =\n"
            "    std::numeric_limits<int>::max() / green_mask;\n"
            "const unsigned int red_norm =\n"
            "    std::numeric_limits<int>::max() / red_mask;"),
         "normalization statements"),
        (palette_saturation_probe.replace("int i = 10", "int i = 0"),
         "entry-10 loop"),
        (palette_saturation_probe.replace(
            "RGBToHSV(r, g, b, &h, &s, &v);",
            "s = static_cast<float>(g);"),
         "RGBToHSV boundary"),
        (palette_saturation_probe.replace("amount <= 1.0f",
                                          "amount < 1.0f"),
         "<=1 multiply"),
        (palette_saturation_probe.replace(
            "HSVToRGB(h, s, v, &r, &g, &b);", "r = g = b;"),
         "HSVToRGB"),
    )
    for probe, description in palette_saturation_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, palette_saturation_key)):
            failures.append("broken TPalette16::AdjustSaturation "
                            + description + " source shape passed")
    palette_hsv_key = ("palette.obj", 0x10B484)
    palette_hsv_probe = """\
const unsigned int red_norm =
    std::numeric_limits<int>::max() / red_mask;
const unsigned int green_norm =
    std::numeric_limits<int>::max() / green_mask;
const unsigned int blue_norm =
    std::numeric_limits<int>::max() / blue_mask;
for (int i = 10; i < 256; ++i) {
    unsigned int r = (data[i] & red_mask) * red_norm;
    unsigned int g = (data[i] & green_mask) * green_norm;
    unsigned int b = (data[i] & blue_mask) * blue_norm;
    float h;
    float s;
    float v;
    RGBToHSV(r, g, b, &h, &s, &v);
    if (hue_adjust >= 0.0f) {
        float delta = hue - h;
        h += delta * hue_adjust;
        if (fabs(delta) > 0.5) {
            if (delta > 0.0) {
                h += 1.0f - hue_adjust;
            } else {
                h += hue_adjust;
            }
            if (h >= 1.0) {
                h -= 1.0;
            }
        }
    }
    if (saturation_adjust >= 0.0f) {
        if (saturation_adjust <= 1.0f) {
            s *= saturation_adjust;
        } else {
            s = 1.0f - (1.0f - s) / saturation_adjust;
        }
    }
    if (value_adjust >= 0.0) {
        if (value_adjust <= 1.0f) {
            v *= value_adjust;
        } else {
            v = 1.0f - (1.0f - v) / value_adjust;
        }
    }
    HSVToRGB(h, s, v, &r, &g, &b);
    data[i] = static_cast<unsigned short>(
        ((r / red_norm) & red_mask) |
        ((g / green_norm) & green_mask) |
        ((b / blue_norm) & blue_mask));
}
"""
    if contract_violations(palette_hsv_probe, palette_hsv_key):
        failures.append("aligned TPalette16::AdjustHSV source shape did not pass")
    palette_hsv_mutations = (
        (palette_hsv_probe.replace("int i = 10", "int i = 0"),
         "entry-10 loop"),
        (palette_hsv_probe.replace(
            "RGBToHSV(r, g, b, &h, &s, &v);", "h = s = v = 0.0f;"),
         "RGBToHSV helper"),
        (palette_hsv_probe.replace("h += delta * hue_adjust;",
                                   "h = hue * hue_adjust;"),
         "hue interpolation"),
        (palette_hsv_probe.replace("fabs(delta) > 0.5",
                                   "fabs(delta) < 0.5"),
         "shortest-path wrap"),
        (palette_hsv_probe.replace(
            "h += 1.0f - hue_adjust;", "h += hue_adjust;", 1),
         "wrap choice"),
        (palette_hsv_probe.replace(
            "if (saturation_adjust >= 0.0f)",
            "if (value_adjust >= 0.0f)", 1),
         "independent saturation"),
        (palette_hsv_probe.replace("value_adjust >= 0.0",
                                   "value_adjust >= 0.0f"),
         "independent saturation then value guards"),
        (palette_hsv_probe.replace(
            "HSVToRGB(h, s, v, &r, &g, &b);", "r = g = b;"),
         "HSVToRGB helper"),
    )
    for probe, description in palette_hsv_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, palette_hsv_key)):
            failures.append("broken TPalette16::AdjustHSV " + description
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
    palette24_hsv_key = ("palette.obj", 0x10BFD4)
    palette24_hsv_probe = """\
const unsigned int red_norm =
    std::numeric_limits<int>::max() / 255;
const unsigned int green_norm =
    std::numeric_limits<int>::max() / 255;
const unsigned int blue_norm =
    std::numeric_limits<int>::max() / 255;
for (int i = 10; i < 256; ++i) {
    unsigned int r = colors.data[i][0] * red_norm;
    unsigned int g = colors.data[i][1] * green_norm;
    unsigned int b = colors.data[i][2] * blue_norm;
    float h;
    float s;
    float v;
    RGBToHSV(r, g, b, &h, &s, &v);
    if (hue_adjust >= 0.0f) {
        float delta = hue - h;
        h += delta * hue_adjust;
        if (fabs(delta) > 0.5) {
            if (delta > 0.0) {
                h += 1.0f - hue_adjust;
            } else {
                h += hue_adjust;
            }
            if (h >= 1.0) {
                h -= 1.0;
            }
        }
    }
    if (value_adjust >= 0.0) {
        if (value_adjust <= 1.0f) {
            v *= value_adjust;
        } else {
            v = 1.0f - (1.0f - v) / value_adjust;
        }
    }
    if (saturation_adjust >= 0.0f) {
        if (saturation_adjust <= 1.0f) {
            s *= saturation_adjust;
        } else if (v > 0.75 && s < 0.25) {
            s = (1.0f - v) * s * saturation_adjust * 4.0f;
        } else {
            s = 1.0f - (1.0f - s) / saturation_adjust;
        }
    }
    HSVToRGB(h, s, v, &r, &g, &b);
    colors.data[i][0] = static_cast<unsigned char>(r / red_norm);
    colors.data[i][1] = static_cast<unsigned char>(g / green_norm);
    colors.data[i][2] = static_cast<unsigned char>(b / blue_norm);
}
"""
    if contract_violations(palette24_hsv_probe, palette24_hsv_key):
        failures.append(
            "aligned TPalette24::AdjustHSV source shape did not pass")
    palette24_hsv_mutations = (
        (palette24_hsv_probe.replace("int i = 10", "int i = 0"),
         "entry-10 loop"),
        (palette24_hsv_probe.replace(
            "RGBToHSV(r, g, b, &h, &s, &v);", "h = s = v = 0.0f;"),
         "RGBToHSV helper"),
        (palette24_hsv_probe.replace("h += delta * hue_adjust;",
                                     "h = hue * hue_adjust;"),
         "hue interpolation"),
        (palette24_hsv_probe.replace("fabs(delta) > 0.5",
                                     "fabs(delta) < 0.5"),
         "shortest-path wrap"),
        (palette24_hsv_probe.replace("value_adjust >= 0.0",
                                     "value_adjust < 0.0", 1),
         "value adjustment before saturation adjustment"),
        (palette24_hsv_probe.replace("v > 0.75 && s < 0.25",
                                     "v > 0.5 && s < 0.25"),
         "dark/high-value saturation arm"),
        (palette24_hsv_probe.replace("* 4.0f;", "* 2.0f;"),
         "dark/high-value saturation arm"),
        (palette24_hsv_probe.replace(
            "HSVToRGB(h, s, v, &r, &g, &b);", "r = g = b;"),
         "HSVToRGB helper"),
        (palette24_hsv_probe.replace("b / blue_norm", "b / red_norm"),
         "three ordered"),
    )
    for probe, description in palette24_hsv_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, palette24_hsv_key)):
            failures.append("broken TPalette24::AdjustHSV " + description
                            + " source shape passed")
    rgb_to_hsv_key = ("palette.obj", 0x10C370)
    rgb_to_hsv_probe = """\
static const float red_hue = 0.0f;
const unsigned int max =
    (r > g ? r : g) > b ? (r > g ? r : g) : b;
const unsigned int min =
    (r < g ? r : g) < b ? (r < g ? r : g) : b;
*v = static_cast<float>(max) / std::numeric_limits<int>::max();
*s = max ? static_cast<float>(max - min) / static_cast<float>(max)
         : 0.0f;
if (max - min) {
    float rc = static_cast<float>(max - r) /
               static_cast<float>(max - min);
    float gc = static_cast<float>(max - g) /
               static_cast<float>(max - min);
    float bc = static_cast<float>(max - b) /
               static_cast<float>(max - min);
    if (r == max) {
        *h = (bc - gc) / 6.0f + red_hue;
    } else if (g == max) {
        *h = (rc - bc) / 6.0f + 1.0f / 3.0f;
    } else {
        *h = (gc - rc) / 6.0f + 2.0f / 3.0f;
    }
    if (*h < 0.0f) {
        *h += 1.0f;
    }
} else {
    *h = 0.0f;
}
"""
    if contract_violations(rgb_to_hsv_probe, rgb_to_hsv_key):
        failures.append("aligned RGBToHSV source shape did not pass")
    rgb_to_hsv_mutations = (
        (rgb_to_hsv_probe.replace(
            "static const float red_hue = 0.0f;", ""),
         "static red_hue"),
        (rgb_to_hsv_probe.replace(
            "(r > g ? r : g) > b ? (r > g ? r : g) : b", "r"),
         "nested extrema"),
        (rgb_to_hsv_probe.replace("*s = max ?", "*s = (max != 0) ?"),
         "single saturation ternary"),
        (rgb_to_hsv_probe.replace("if (max - min)",
                                  "if (max != min)"),
         "arithmetic chroma guard"),
        (rgb_to_hsv_probe.replace(
            "float rc =", "float bc_temp =", 1),
         "rc, gc, bc"),
        (rgb_to_hsv_probe.replace("+ red_hue", "+ 0.0f"),
         "red_hue load"),
        (rgb_to_hsv_probe.replace("} else if (g == max) {",
                                  "} else if (b == max) {"),
         "red, green, blue sector order"),
        (rgb_to_hsv_probe.replace("*h += 1.0f;", "*h = 0.0f;", 1),
         "negative wrap"),
    )
    for probe, description in rgb_to_hsv_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, rgb_to_hsv_key)):
            failures.append("broken RGBToHSV " + description
                            + " source shape passed")
    hsv_to_rgb_key = ("palette.obj", 0x10C564)
    hsv_to_rgb_probe = """\
if (s != 0.0f) {
    const float f = static_cast<float>(fmod(h * 6.0f, 1.0));
    v *= static_cast<float>(std::numeric_limits<int>::max());
    const float p = v * (1.0f - s);
    const float q = v * (1.0f - s * f);
    const float t = v * (1.0f - s * (1.0f - f));
    switch (static_cast<int>(h * 6.0f)) {
    case HSV_RED_SECTOR:
        *r = ftol(v); *g = ftol(t); *b = ftol(p); break;
    case HSV_YELLOW_SECTOR:
        *r = ftol(q); *g = ftol(v); *b = ftol(p); break;
    case HSV_GREEN_SECTOR:
        *r = ftol(p); *g = ftol(v); *b = ftol(t); break;
    case HSV_CYAN_SECTOR:
        *r = ftol(p); *g = ftol(q); *b = ftol(v); break;
    case HSV_BLUE_SECTOR:
        *r = ftol(t); *g = ftol(p); *b = ftol(v); break;
    case HSV_MAGENTA_SECTOR:
        *r = ftol(v); *g = ftol(p); *b = ftol(q); break;
    }
} else {
    *r = *g = *b = ftol(
        v * static_cast<float>(std::numeric_limits<int>::max()));
}
"""
    if contract_violations(hsv_to_rgb_probe, hsv_to_rgb_key):
        failures.append("aligned HSVToRGB source shape did not pass")
    hsv_to_rgb_mutations = (
        (hsv_to_rgb_probe.replace("if (s != 0.0f)", "if (s == 0.0f)"),
         "chromatic scope"),
        (hsv_to_rgb_probe.replace("fmod(h * 6.0f, 1.0)", "h * 6.0f"),
         "fmod helper boundary"),
        (hsv_to_rgb_probe.replace(
            "const float p = v * (1.0f - s);\n"
            "    const float q = v * (1.0f - s * f);",
            "const float q = v * (1.0f - s * f);\n"
            "    const float p = v * (1.0f - s);"),
         "p/q/t statement order"),
        (hsv_to_rgb_probe.replace("*g = ftol(t);", "*g = ftol(q);", 1),
         "hue-sector channel mappings"),
        (hsv_to_rgb_probe.replace("*r = ftol(v);",
                                  "*r = static_cast<unsigned int>(v);", 1),
         "nineteen"),
        (hsv_to_rgb_probe.replace("*r = *g = *b = ftol(",
                                  "*r = *g; *g = *b; *b = ftol("),
         "right-associated write"),
    )
    for probe, description in hsv_to_rgb_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, hsv_to_rgb_key)):
            failures.append("broken HSVToRGB " + description
                            + " source shape passed")
    update_go_key = ("singleselectionwindow.obj", 0x1480CC)
    update_go_probe = """\
CGameHeaderInfoInitMsgEx initMsg(
    gUnnamed69fbe8->gameVersion,
    gUnnamed69fbe8->HeadersA.size(),
    gUnnamed69fbe8->m_flag64);
TransmitRemoteDataDPID(&initMsg, m_dpid, false, true);
"""
    if contract_violations(update_go_probe, update_go_key):
        failures.append("aligned CNewPlayerUpdateProc::Go shape did not pass")
    update_go_mutations = (
        ("TSingleSelectionWindow* win = gUnnamed69fbe8;\n"
         + update_go_probe.replace("gUnnamed69fbe8->", "win->"),
         "sole initMsg local"),
        (update_go_probe.replace("HeadersA", "TransferHeaders"),
         "version, HeadersA count, and network-mode arguments in order"),
        (update_go_probe.replace("false, true", "true, false"),
         "constructor then TransmitRemoteDataDPID statement order"),
    )
    for probe, description in update_go_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, update_go_key)):
            failures.append("broken CNewPlayerUpdateProc::Go "
                            + description + " source shape passed")
    update_tick_key = ("singleselectionwindow.obj", 0x148130)
    update_tick_probe = """\
if (GameTime::ElapsedSince(m_lastSendTime) < 75)
    return;
if (m_nextHeader < gUnnamed69fbe8->HeadersA.size()) {
    for (int i = 0; i < 5; ++i) {
        if (gUnnamed69fbe8->m_flag64) {
            CGameHeaderInfoMsg msg(
                0, m_nextHeader,
                &gUnnamed69fbe8->HeadersA[m_nextHeader]);
            msg.RemoteFn_00512C80(m_dpid, 1, 1);
        } else {
            CMapFileNameMsg msg(
                0, m_nextHeader,
                gUnnamed69fbe8->HeadersA[m_nextHeader].setup.filename,
                gUnnamed69fbe8->HeadersA[m_nextHeader].setup.alignment,
                gUnnamed69fbe8->HeadersA[m_nextHeader].fileTime);
            TransmitRemoteDataDPID(&msg, m_dpid, true, false);
        }
        ++m_nextHeader;
        if (static_cast<int>(m_requests.size()) != 0)
            HandleRequests();
        if (m_nextHeader >= gUnnamed69fbe8->HeadersA.size()) {
            RequestConfirmation();
            break;
        }
    }
} else if (static_cast<int>(m_requests.size()) != 0) {
    HandleRequests();
    RequestConfirmation();
}
m_lastSendTime = GameTime::Get();
"""
    if contract_violations(update_tick_probe, update_tick_key):
        failures.append("aligned CNewPlayerUpdateProc::Tick shape did not pass")
    update_tick_mutations = (
        (update_tick_probe.replace(
            "GameTime::ElapsedSince(m_lastSendTime)",
            "GameTime::Get() - m_lastSendTime"),
         "ElapsedSince throttle"),
        (update_tick_probe.replace("HeadersA", "TransferHeaders"),
         "CGameHeaderInfoMsg/CMapFileNameMsg alternatives"),
        (update_tick_probe.replace(
            "++m_nextHeader;\n        if (static_cast<int>",
            "if (static_cast<int>", 1),
         "increment, HandleRequests, exhaustion test"),
        (update_tick_probe.replace("HandleRequests();", "m_requests.clear();"),
         "increment, HandleRequests, exhaustion test"),
        (update_tick_probe.replace(
            "HandleRequests();\n    RequestConfirmation();",
            "RequestConfirmation();\n    HandleRequests();"),
         "exhausted-list request drain before confirmation"),
        (update_tick_probe.replace(
            "m_lastSendTime = GameTime::Get();", "m_lastSendTime = 0;"),
         "refreshes m_lastSendTime last"),
    )
    for probe, description in update_tick_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, update_tick_key)):
            failures.append("broken CNewPlayerUpdateProc::Tick "
                            + description + " source shape passed")
    finish_key = ("singleselectionwindow.obj", 0x1484C8)
    finish_probe = """\
CGameHeaderInfoEndMsg endMsg;
TransmitRemoteDataDPID(&endMsg, m_dpid, false, true);
CSetFilterMsg filterMsg(gUnnamed69fbe8->mapSizeFilter);
TransmitRemoteDataDPID(&filterMsg, m_dpid, false, true);
CScrollMsg scrollMsg(gUnnamed69fbe8->currentMap,
                     gUnnamed69fbe8->currentIndex);
TransmitRemoteDataDPID(&scrollMsg, m_dpid, false, true);
gUnnamed69fbe8->SendSetupInfo(m_dpid);
if (gUnnamed69fbe8->inAdvancedOptions) {
    CClickMsg clickMsg(129);
    TransmitRemoteDataDPID(&clickMsg, m_dpid, false, true);
} else {
    if (gUnnamed69fbe8->inScenarioOptions) {
        CClickMsg clickMsg(128);
        TransmitRemoteDataDPID(&clickMsg, m_dpid, false, true);
    } else if (gUnnamed69fbe8->inFilterOptions) {
        CClickMsg clickMsg(130);
        TransmitRemoteDataDPID(&clickMsg, m_dpid, false, true);
    }
}
CClickMsg clickMsg(gpGame->setup.difficulty + 107);
TransmitRemoteDataDPID(&clickMsg, m_dpid, false, true);
gUnnamed69fbe8->SendPlayerPositions(m_dpid);
gUnnamed69fbe8->CheckFaces();
gUnnamed69fbe8->SendPlayerFaces();
"""
    if contract_violations(finish_probe, finish_key):
        failures.append("aligned CNewPlayerUpdateProc::Finish shape did not pass")
    finish_mutations = (
        (finish_probe.replace(
            "CGameHeaderInfoEndMsg endMsg;\n"
            "TransmitRemoteDataDPID(&endMsg, m_dpid, false, true);\n",
            ""), "end, filter, scroll, and setup messages"),
        (finish_probe.replace("inScenarioOptions", "chatShowing"),
         "advanced and scenario pane click-message scopes"),
        (finish_probe.replace(
            "gUnnamed69fbe8->CheckFaces();\n"
            "gUnnamed69fbe8->SendPlayerFaces();",
            "gUnnamed69fbe8->SendPlayerFaces();\n"
            "gUnnamed69fbe8->CheckFaces();"),
         "final click, player-position send, face validation"),
    )
    for probe, description in finish_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, finish_key)):
            failures.append("broken CNewPlayerUpdateProc::Finish "
                            + description + " source shape passed")

    send_faces_key = ("singleselectionwindow.obj", 0x1425F0)
    send_faces_probe = """\
for (int i = 1; i < 8; ++i) {
    CNetPlayerHandlerPlayer* pPlayer = &m_players.humanPlayers[i];
    if (pPlayer->IsHuman() && pPlayer->playerPos != -1) {
        CRequestHeroFaceReplyMsg msg(pPlayer->playerPos,
                                     pPlayer->heroIndex);
        TransmitRemoteDataDPID(&msg, 0, false, true);
    }
}
"""
    if contract_violations(send_faces_probe, send_faces_key):
        failures.append("aligned SendPlayerFaces shape did not pass")
    for probe in (
            send_faces_probe.replace("int i = 1", "int i = 0"),
            send_faces_probe.replace("pPlayer->IsHuman() && ", ""),
            send_faces_probe.replace("pPlayer->heroIndex", "i"),
            send_faces_probe.replace("&msg, 0, false, true",
                                     "&msg, m_dpid, false, true")):
        if not contract_violations(probe, send_faces_key):
            failures.append("broken SendPlayerFaces source shape passed")

    constructor_body_controls = (
        (("singleselectionwindow.obj", 0x147ACC), "", "m_flag = 0;"),
        (("singleselectionwindow.obj", 0x147AF4),
         "m_setup = *setup;", "memcpy(&m_setup, setup, sizeof(m_setup));"),
        (("singleselectionwindow.obj", 0x147BF0),
         "m_map = map;\nm_index = index;",
         "m_index = index;\nm_map = map;"),
        (("singleselectionwindow.obj", 0x147DA4),
         "m_size = size;", "m_size = 0;"),
        (("singleselectionwindow.obj", 0x147F28),
         "m_widgetId = widgetId;", "m_widgetId = 0;"),
    )
    for key, aligned, broken in constructor_body_controls:
        if contract_violations(aligned, key):
            failures.append("aligned Finish message constructor did not pass")
        if not contract_violations(broken, key):
            failures.append("broken Finish message constructor passed")
    finish_constructor_source_probe = """\
CGameHeaderInfoEndMsg::CGameHeaderInfoEndMsg()
    : CNetMsg(RS_GAME_HEADER_INFO_END, sizeof(CGameHeaderInfoEndMsg))
{
}
CNewSetupInfoMsg::CNewSetupInfoMsg(SGameSetupOptions* setup)
    : CNetMsg(RS_NEW_SETUP_INFO, sizeof(CNewSetupInfoMsg))
{
    m_setup = *setup;
}
CScrollMsg::CScrollMsg(int map, int index)
    : CNetMsg(RS_SCROLL, sizeof(CScrollMsg))
{
    m_map = map;
    m_index = index;
}
CSetFilterMsg::CSetFilterMsg(int size)
    : CNetMsg(RS_SET_FILTER, sizeof(CSetFilterMsg))
{
    m_size = size;
}
CClickMsg::CClickMsg(int widgetId)
    : CNetMsg(RS_CLICK, sizeof(CClickMsg))
{
    m_widgetId = widgetId;
}
"""
    if finish_message_constructor_contract_violations(
            finish_constructor_source_probe):
        failures.append("aligned Finish constructor file contract did not pass")
    finish_constructor_mutations = (
        finish_constructor_source_probe.replace(
            "RS_GAME_HEADER_INFO_END", "RS_HEADER_CONFIRM", 1),
        finish_constructor_source_probe.replace(
            "m_setup = *setup;", "memcpy(&m_setup, setup, sizeof(m_setup));"),
        finish_constructor_source_probe.replace(
            "m_map = map;\n    m_index = index;",
            "m_index = index;\n    m_map = map;"),
        finish_constructor_source_probe.replace(
            "CClickMsg::CClickMsg", "CClickMsg::Init"),
    )
    if any(not finish_message_constructor_contract_violations(probe)
           for probe in finish_constructor_mutations):
        failures.append("broken Finish constructor file contract passed")
    new_player_key = ("singleselectionwindow.obj", 0x14870C)
    new_player_probe = """\
int index = GetFirstAvailable();
if (index != -1) {
    m_procs[index] = new CNewPlayerUpdateProc(dpid);
    m_procs[index]->Go();
}
"""
    if contract_violations(new_player_probe, new_player_key):
        failures.append("aligned CNewPlayerUpdateMan::NewPlayer shape did not pass")
    new_player_mutations = (
        (new_player_probe.replace("int index", "int i").replace(
            "index", "i"), "sole index local"),
        (new_player_probe.replace(
            "int index = GetFirstAvailable();", "int index = 0;"),
         "GetFirstAvailable boundary"),
        (new_player_probe.replace(
            "m_procs[index] = new CNewPlayerUpdateProc(dpid);\n"
            "    m_procs[index]->Go();",
            "m_procs[index]->Go();\n"
            "    m_procs[index] = new CNewPlayerUpdateProc(dpid);"),
         "construction then virtual Go"),
        (new_player_probe.replace("m_procs[index]->Go();",
                                  "CNewPlayerUpdateProc::Go();"),
         "virtual Go statement"),
    )
    for probe, description in new_player_mutations:
        if not any(description in rule.description for rule in
                   contract_violations(probe, new_player_key)):
            failures.append("broken CNewPlayerUpdateMan::NewPlayer "
                            + description + " shape passed")
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
    can_shoot_key = ("army.obj", 0x47C04)
    can_shoot_probe = """\
if (enemy_is_adjacent(excluded))
    bCanShoot = 0;
"""
    if contract_violations(can_shoot_probe, can_shoot_key):
        failures.append("aligned can_shoot owner-sensitive helper did not pass")
    flattened_can_shoot_probes = (
        can_shoot_probe.replace(
            "enemy_is_adjacent(excluded)",
            "gpCombatManager->enemy_is_adjacent(this, gridIndex, excluded)"),
        can_shoot_probe.replace(
            "enemy_is_adjacent(excluded)", "enemy_is_adjacent(0)"),
    )
    if any(not contract_violations(probe, can_shoot_key)
           for probe in flattened_can_shoot_probes):
        failures.append("flattened can_shoot owner-sensitive helper passed")
    enemy_adjacent_key = ("army.obj", 0x47C74)
    enemy_adjacent_probe = """\
if (gpCombatManager->enemy_is_adjacent(this, gridIndex, excluded))
    return 1;
if (Is(1u))
    return gpCombatManager->enemy_is_adjacent(
        this, get_second_grid_index(), excluded);
return 0;
"""
    if contract_violations(enemy_adjacent_probe, enemy_adjacent_key):
        failures.append("aligned enemy_is_adjacent helper chain did not pass")
    flattened_enemy_adjacent_probes = (
        enemy_adjacent_probe.replace("Is(1u)", "creatureId & 1"),
        enemy_adjacent_probe.replace(
            "get_second_grid_index()", "gridIndex + (facing ? 1 : -1)"),
    )
    if any(not contract_violations(probe, enemy_adjacent_key)
           for probe in flattened_enemy_adjacent_probes):
        failures.append("flattened enemy_is_adjacent helper chain passed")
    get_berserk_targets_key = ("army.obj", 0x4A348)
    get_berserk_targets_probe = """\
if (can_shoot(0))
    canShoot = 1;
value = gpSearchArray->get_hex(other->gridIndex)->cost;
if (value < best)
    armies.clear();
armies.push_back(other);
"""
    if contract_violations(get_berserk_targets_probe,
                           get_berserk_targets_key):
        failures.append("aligned get_berserk_targets helper shape did not pass")
    flattened_get_berserk_targets_probes = (
        get_berserk_targets_probe.replace(
            "can_shoot(0)", "can_shoot_flagform(0)"),
        get_berserk_targets_probe.replace(
            "gpSearchArray->get_hex(other->gridIndex)->cost",
            "gpSearchArray->cellData[other->gridIndex].cost"),
        get_berserk_targets_probe.replace(
            "armies.clear()", "armies.erase(armies.begin(), armies.end())"),
        get_berserk_targets_probe.replace(
            "armies.push_back(other)", "armies.insert(armies.end(), other)"),
    )
    if any(not contract_violations(probe, get_berserk_targets_key)
           for probe in flattened_get_berserk_targets_probes):
        failures.append("flattened get_berserk_targets helper shape passed")
    get_second_grid_index_key = ("army.obj", 0x4B354)
    get_second_grid_index_probe = """\
if (!Is(1u))
    return gridIndex;
return gridIndex + OffsetToFront(-1);
"""
    if contract_violations(get_second_grid_index_probe,
                           get_second_grid_index_key):
        failures.append("aligned get_second_grid_index helper shape did not pass")
    flattened_get_second_grid_index_probes = (
        get_second_grid_index_probe.replace("Is(1u)", "creatureId & 1"),
        get_second_grid_index_probe.replace(
            "OffsetToFront(-1)", "(facing ? 1 : -1)"),
    )
    if any(not contract_violations(probe, get_second_grid_index_key)
           for probe in flattened_get_second_grid_index_probes):
        failures.append("flattened get_second_grid_index helper shape passed")
    go_berserk_key = ("army.obj", 0x4A480)
    go_berserk_probe = """\
if (can_shoot(0)) {
    if (target->get_owning_side() == get_owning_side())
        field_53dc[get_owning_side()] = 1;
    return;
}
gpCombatManager->berserk_attack(this, target);
"""
    if contract_violations(go_berserk_probe, go_berserk_key):
        failures.append("aligned GoBerserk helper shape did not pass")
    flattened_go_berserk_probes = (
        go_berserk_probe.replace("can_shoot(0)", "shotsLeft > 0"),
        go_berserk_probe.replace(
            "target->get_owning_side()", "target->combatSide"),
        go_berserk_probe.replace("field_53dc[get_owning_side()]",
                                 "field_53dc[combatSide]"),
        go_berserk_probe.replace(
            "    return;\n}\ngpCombatManager->berserk_attack(this, target);",
            "} else {\n    gpCombatManager->berserk_attack(this, target);\n}"),
    )
    if any(not contract_violations(probe, go_berserk_key)
           for probe in flattened_go_berserk_probes):
        failures.append("flattened GoBerserk helper shape passed")
    attack_hex_key = ("army.obj", 0x4A7AC)
    attack_hex_probe = """\
int direction = get_attack_direction(target);
if (direction >= 0)
    do_attack(direction);
"""
    if contract_violations(attack_hex_probe, attack_hex_key):
        failures.append("aligned attack_hex helper shape did not pass")
    flattened_attack_hex_probes = (
        attack_hex_probe.replace(
            "get_attack_direction(target)",
            "get_attack_direction(gridIndex, target)"),
        """\
int direction = -1;
for (int i = 0; i < 8; ++i) {
    long adjacent = get_adjacent_hex(gridIndex, i);
    if (target == gpCombatManager->cells[adjacent].get_army())
        direction = i;
}
""",
    )
    if any(not contract_violations(probe, attack_hex_key)
           for probe in flattened_attack_hex_probes):
        failures.append("flattened attack_hex helper shape passed")
    do_attack_direction_key = ("army.obj", 0x46FB0)
    do_attack_direction_probe = """\
long counter_direction = armyToAttack->get_attack_direction(this);
if (armyToAttack->NeedToTurn(counter_direction)) {}
"""
    if contract_violations(do_attack_direction_probe,
                           do_attack_direction_key):
        failures.append("aligned do_attack direction helper did not pass")
    flattened_do_attack_direction_probes = (
        do_attack_direction_probe.replace(
            "get_attack_direction(this)",
            "get_attack_direction(armyToAttack->gridIndex, this)"),
        """\
long counter_direction = -1;
for (long direction = 0; direction < 8; ++direction) {
    long hex = armyToAttack->get_adjacent_hex(
        armyToAttack->gridIndex, direction);
    if (this == gpCombatManager->cells[hex].get_army())
        counter_direction = direction;
}
""",
    )
    if any(not contract_violations(probe, do_attack_direction_key)
           for probe in flattened_do_attack_direction_probes):
        failures.append("flattened do_attack direction helper passed")
    do_post_attack_key = ("army.obj", 0x46658)
    do_post_attack_probe = """\
switch (creatureType) {
case CREATURE_VAMPIRE_LORD:
    long dead_vampires = 0;
    long missing_life = hitPoints * (origNumTroops - numTroops)
                        + topCreatureDamage;
    long damage_recovered = min(iDamage, total_life);
    damage_recovered = min(damage_recovered, missing_life);
    text = format_string((*gpGeneralText)[362]);
    text = format_string((*gpGeneralText)[363]);
    text += (*gpGeneralText)[364];
    text += format_string((*gpGeneralText)[365]);
    break;
case CREATURE_MIGHTY_GORGON: {
    int stares = 0;
    if (target->Is(1u << 4)) {
        for (long i = 0; i < numTroops; i++) {
            if (Random(1, 100) <= 10)
                stares++;
        }
        long dead = min(stares, target->numTroops);
        dead = min(dead, (numTroops + 9) / 10);
        text = format_string((*gpGeneralText)[119]);
        text = format_string((*gpGeneralText)[120]);
    }
    break;
}
case CREATURE_THUNDERBIRD:
    text = format_string((*gpGeneralText)[368]);
    break;
}
"""
    if contract_violations(do_post_attack_probe, do_post_attack_key):
        failures.append("aligned do_post_attack source shape did not pass")
    flattened_do_post_attack_probes = (
        do_post_attack_probe.replace(
            "(*gpGeneralText)[362]", "gpGeneralText->GetText(362)"),
        do_post_attack_probe.replace(
            "long damage_recovered = min(iDamage, total_life);\n"
            "    damage_recovered = min(damage_recovered, missing_life);",
            "long damage_recovered = "
            "_cpp_min(_cpp_min(iDamage, total_life), missing_life);"),
        do_post_attack_probe.replace(
            "for (long i = 0; i < numTroops; i++) {\n"
            "            if (Random(1, 100) <= 10)\n"
            "                stares++;\n"
            "        }",
            "stares = roll_death_stares(this);"),
        do_post_attack_probe.replace(
            "    int stares = 0;\n"
            "    if (target->Is(1u << 4)) {",
            "    if (target->Is(1u << 4)) {\n"
            "        int stares = 0;"),
        do_post_attack_probe.replace(
            "long dead = min(stares, target->numTroops);\n"
            "        dead = min(dead, (numTroops + 9) / 10);",
            "long dead = min(min(stares, target->numTroops), "
            "(numTroops + 9) / 10);"),
    )
    if any(not contract_violations(probe, do_post_attack_key)
           for probe in flattened_do_post_attack_probes):
        failures.append("flattened do_post_attack source shape passed")
    automate_first_aid_key = ("command.obj", 0x6B12C)
    automate_first_aid_probe = """\
field_3c = 11;
field_44 = armies[side][best_index].gridIndex;
field_40 = -1;
return 1;
"""
    if contract_violations(automate_first_aid_probe,
                           automate_first_aid_key):
        failures.append("aligned automate_first_aid_tent tail did not pass")
    automate_first_aid_mutations = (
        automate_first_aid_probe.replace(
            "field_44 = armies[side][best_index].gridIndex;",
            "int grid_index = armies[side][best_index].gridIndex;\n"
            "field_44 = grid_index;"),
        automate_first_aid_probe.replace(
            "field_44 = armies[side][best_index].gridIndex;\n"
            "field_40 = -1;",
            "field_40 = -1;\n"
            "field_44 = armies[side][best_index].gridIndex;"),
    )
    if any(not contract_violations(probe, automate_first_aid_key)
           for probe in automate_first_aid_mutations):
        failures.append("flattened automate_first_aid_tent tail passed")
    automate_catapult_key = ("command.obj", 0x6AF98)
    automate_catapult_probe = """\
valid_wall_target(target);
valid_wall_target(towers[index]);
valid_wall_target(target);
get_wall_strength(walls[i]);
get_wall_strength(walls[i]);
get_wall_strength(walls[index]);
SRandom(1, count);
get_secondary_skill(eSecSkillSiegeBallistics);
field_3c = 9;
field_44 = wallTargets[target].target_hex;
field_40 = -1;
return 1;
"""
    if contract_violations(automate_catapult_probe,
                           automate_catapult_key):
        failures.append("aligned automate_catapult shape did not pass")
    automate_catapult_mutations = (
        automate_catapult_probe.replace(
            "valid_wall_target(target);\n", "", 1),
        automate_catapult_probe.replace(
            "get_wall_strength(walls[i]);\n", "", 1),
        automate_catapult_probe.replace("SRandom(1, count);",
                                         "Random(1, count);"),
        automate_catapult_probe.replace(
            "get_secondary_skill(eSecSkillSiegeBallistics);",
            "skillLevel[eSecSkillSiegeBallistics];"),
        automate_catapult_probe.replace(
            "field_44 = wallTargets[target].target_hex;\nfield_40 = -1;",
            "field_40 = -1;\nfield_44 = wallTargets[target].target_hex;"),
    )
    if any(not contract_violations(probe, automate_catapult_key)
           for probe in automate_catapult_mutations):
        failures.append("flattened automate_catapult shape passed")
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
    carcass_claim_probe = """\
#if 0
VA(0x00401000, 0x10) // dc 0x100
void army::CheckLuck() { get_controller(); }
#endif
void army::CheckLuck() { get_controller(); }
"""
    carcass_mask = _source.mask(carcass_claim_probe)
    carcass_body = _body_for_claim(
        carcass_mask, _line_starts(carcass_claim_probe),
        2, "army::CheckLuck")
    if carcass_body is None or missing_from_body(
            carcass_claim_probe[carcass_body[0] + 1:carcass_body[1]],
            ["army::get_controller"]):
        failures.append("RVA-order carcass claim missed its active definition")
    case_distinct_claim_probe = """\
#if 0
VA(0x00401000, 0x10) // dc 0x100
void MoveHero() { DemobilizeCurrHero(); }
#endif
void move_hero() { IncrementHourGlass(); }
"""
    if _body_for_claim(
            _source.mask(case_distinct_claim_probe),
            _line_starts(case_distinct_claim_probe), 2, "MoveHero") \
            is not None:
        failures.append(
            "claim-only function bound to case-distinct next definition")
    flattened_claim_probe = """\
#if 0
VA(0x00401000, 0x10) // dc 0x100
void army::CheckLuck() { get_controller(); }
#endif
void army::CheckLuck() { iLuckStatus = 0; }
"""
    flattened_mask = _source.mask(flattened_claim_probe)
    flattened_body = _body_for_claim(
        flattened_mask, _line_starts(flattened_claim_probe),
        2, "army::CheckLuck")
    if flattened_body is None or not missing_from_body(
            flattened_claim_probe[
                flattened_body[0] + 1:flattened_body[1]],
            ["army::get_controller"]):
        failures.append("flattened RVA-order claim helper passed")
    definition_owner = PROVEN_DEFINITION_OWNERS[
        ("command.obj", 0x70AD0)]
    definition_owner_probe = """\
class CMessageKill {
    ~CMessageKill()
    {
        if (m_pNetMsg)
            DestroyMsg(m_pNetMsg);
    }
};
"""
    owner_definition = _definition_for_owner(
        _source.mask(definition_owner_probe), definition_owner)
    if owner_definition is None or missing_from_body(
            definition_owner_probe[
                owner_definition.body_open + 1:owner_definition.body_close],
            ["DestroyMsg"]):
        failures.append("mapped header definition owner was not audited")
    flattened_owner_probe = definition_owner_probe.replace(
        "DestroyMsg(m_pNetMsg);", "delete m_pNetMsg;")
    flattened_owner = _definition_for_owner(
        _source.mask(flattened_owner_probe), definition_owner)
    if flattened_owner is None or not missing_from_body(
            flattened_owner_probe[
                flattened_owner.body_open + 1:flattened_owner.body_close],
            ["DestroyMsg"]):
        failures.append("flattened mapped header definition passed")
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
    palette_transfer = PROVEN_CALL_TRANSFERS[
        ("resourcemanager.obj", 0x121EC8, "LODFile::read")]
    palette_transfer_caller = """\
t_lod_file_adapter stream(lodFile);
TAbstractFile* streamInterface = &stream;
streamInterface->Read(header, sizeof(header));
streamInterface->Read(rgba, sizeof(rgba));
"""
    palette_transfer_receiver = """\
return lod_file->read(data, size) ? 0 : size;
"""
    if not transfer_satisfied(
            palette_transfer, palette_transfer_caller,
            palette_transfer_receiver, {palette_transfer.receiver_va}):
        failures.append("exact LOD adapter call transfer did not pass")
    if transfer_satisfied(
            palette_transfer, palette_transfer_caller,
            palette_transfer_receiver, set()):
        failures.append("non-exact LOD adapter receiver passed")
    if transfer_satisfied(
            palette_transfer, palette_transfer_caller,
            palette_transfer_receiver.replace("lod_file->read", "read"),
            {palette_transfer.receiver_va}):
        failures.append("flattened LOD adapter receiver passed")
    if transfer_satisfied(
            palette_transfer,
            palette_transfer_caller.replace(
                "streamInterface->Read(rgba, sizeof(rgba));", ""),
            palette_transfer_receiver, {palette_transfer.receiver_va}):
        failures.append("LOD adapter transfer with one caller read passed")
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
    wrapper_key = ("singleselectionwindow.obj", 0x135AA4)
    wrapper_va = PROVEN_CALL_SPELLINGS[wrapper_key][0].caller_va
    wrapper_body = "SetNewPlayerSlot(&player);"
    canonical = apply_proven_call_spellings(
        wrapper_key, wrapper_body, wrapper_va, {wrapper_va})
    if missing_from_body(canonical,
                         ["CNetPlayerHandler::AddNewPlayer"]):
        failures.append("proved Complete wrapper did not preserve DC helper")
    unproved = apply_proven_call_spellings(
        wrapper_key, wrapper_body, wrapper_va, set())
    if not missing_from_body(unproved,
                             ["CNetPlayerHandler::AddNewPlayer"]):
        failures.append("non-exact Complete wrapper bypassed source gate")
    erased = apply_proven_call_spellings(
        wrapper_key, "return 0;", wrapper_va, {wrapper_va})
    if not missing_from_body(erased,
                             ["CNetPlayerHandler::AddNewPlayer"]):
        failures.append("erased Complete wrapper passed source-shape gate")
    relocation_spelling_key = ("swapmgr.obj", 0x15D150)
    relocation_spelling_va = \
        NONEXACT_RETAIL_PROVEN_CALL_SPELLINGS[
            relocation_spelling_key][0].caller_va
    relocation_spelling_body = """\
if (!our_hero->HeroFn_004E2840(
        gHeroScreenDraggedArtifact.artifactId, slot))
    return;
"""
    relocation_canonical = apply_proven_call_spellings(
        relocation_spelling_key, relocation_spelling_body,
        relocation_spelling_va, set())
    if missing_from_body(relocation_canonical, ["artifactAllowedInSlot"]) \
            or contract_violations(
                relocation_spelling_body, relocation_spelling_key):
        failures.append(
            "relocation-proved Complete call spelling did not pass")
    relocation_omitted = "return;"
    if not missing_from_body(apply_proven_call_spellings(
            relocation_spelling_key, relocation_omitted,
            relocation_spelling_va, set()), ["artifactAllowedInSlot"]):
        failures.append(
            "omitted relocation-proved Complete call spelling passed")
    relocation_old_only = """\
if (!artifactAllowedInSlot(dragged, slot))
    return;
"""
    if not contract_violations(
            relocation_old_only, relocation_spelling_key):
        failures.append("obsolete Dreamcast helper-only spelling passed")
    relocation_flattened = """\
if (!(allowable_slots[slot] & dragged_artifact_mask))
    return;
"""
    if not contract_violations(
            relocation_flattened, relocation_spelling_key):
        failures.append("flattened Complete artifact predicate passed")
    relocation_wrong_arguments = """\
if (!our_hero->HeroFn_004E2840(slot,
        gHeroScreenDraggedArtifact.artifactId))
    return;
"""
    if not contract_violations(
            relocation_wrong_arguments, relocation_spelling_key):
        failures.append(
            "wrong-argument Complete artifact predicate passed")
    removal_key = ("singleselectionwindow.obj", 0x135F04)
    removal_va = PROVEN_REVISION_REMOVALS[removal_key][0].caller_va
    removal_body = """\
ShowWidget(137);
ShowWidget(141);
ShowWidget(190);
ShowWidget(195);
"""
    removed_helpers, removal_descriptions = \
        proven_dc_only_removed_helpers(
            removal_key, removal_body, removal_va, {removal_va})
    if removed_helpers != frozenset(
            PROVEN_REVISION_REMOVALS[removal_key][0].dc_only_helpers) \
            or len(removal_descriptions) != 1:
        failures.append("exact Complete revision removal did not classify")
    if proven_dc_only_removed_helpers(
            removal_key, removal_body, removal_va, set())[0]:
        failures.append("non-exact revision removal classified DC-only")
    if proven_dc_only_removed_helpers(
            removal_key, removal_body.replace("ShowWidget(195);", ""),
            removal_va, {removal_va})[0]:
        failures.append("erased Complete replacement group classified")
    retail_removal_key = ("mapcell.obj", 0xF0DF4)
    retail_removal_va = \
        RETAIL_BYTE_PROVEN_REVISION_REMOVALS[
            retail_removal_key][0].caller_va
    retail_removal_body = """\
char padding[16];
unsigned char isRandomHero;
int HeroID;
int int_buffer;
short short_buffer;
char Owner;
char customName;
int count;
int experience;
int x;
char char_buffer;
char tempText[100] = { 0 };
HeroExtra* hero_data;
if (customName) {
    hero_data->bCustomName = 1;
    strcpy(hero_data->Name, tempText);
}
if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
    experience = int_buffer;
} else {
    bCustomExperience = experienceFlag != 0;
    if (bCustomExperience) {
        infile->Read(&experience, sizeof(experience));
    } else {
        experience = 0;
    }
}
if (HeroID == -1) {
    HeroID = gpGame->GetStartingHeroId(
        alignment, char_buffer, experience);
}
for (x = 0; x < hero_data->NumSecondarySkills; ++x) {
    hero_data->secondarySkill[x] = type;
    hero_data->secondarySkillLevel[x] = level;
}
for (x = 0; x < armyGroup::ARMY_GROUP_SLOT_COUNT; ++x) {
    hero_data->armies[x] = creature;
    hero_data->numTroops[x] = troops;
}
count = 18;
for (x = 0; x < count; ++x) {
    hero_data->artifacts[x].artifactId = artifact;
}
for (x = 0; x < hero_data->numInBackpack; ++x) {
    hero_data->backpack[x].artifactId = artifact;
}
for (x = 0; x < 4; ++x) {
    hero_data->primarySkills[x] = value;
}
hero_data->location = heroObject->get_trigger();
return 0;
"""
    retail_removed, retail_descriptions = \
        retail_proven_dc_only_removed_helpers(
            retail_removal_key, retail_removal_body, retail_removal_va)
    if retail_removed != frozenset(("Random", "strncpy")) \
            or len(retail_descriptions) != 2:
        failures.append(
            "retail-byte-proved readHeroData revision removal did not "
            "classify")
    if contract_violations(retail_removal_body, retail_removal_key):
        failures.append("Complete readHeroData source shape did not pass")
    obsolete_random_body = retail_removal_body.replace(
        "experience = 0;", "experience = Random(0, 50) + 40;")
    if "Random" in retail_proven_dc_only_removed_helpers(
            retail_removal_key, obsolete_random_body,
            retail_removal_va)[0]:
        failures.append("obsolete Dreamcast Random fallback classified out")
    if not contract_violations(obsolete_random_body, retail_removal_key):
        failures.append("obsolete Dreamcast Random fallback passed")
    obsolete_name_body = retail_removal_body.replace(
        "if (customName) {\n"
        "    hero_data->bCustomName = 1;\n"
        "    strcpy(hero_data->Name, tempText);\n"
        "}",
        "hero_data->bCustomName = customName;\n"
        "if (hero_data->bCustomName && !isRandomHero) {\n"
        "    strncpy(hero_data->Name, tempText, 12);\n"
        "}")
    obsolete_name_removed = retail_proven_dc_only_removed_helpers(
            retail_removal_key, obsolete_name_body, retail_removal_va)[0]
    if "strncpy" in obsolete_name_removed:
        failures.append("obsolete Dreamcast name copy classified out")
    if not contract_violations(obsolete_name_body, retail_removal_key):
        failures.append("obsolete Dreamcast name copy passed")
    process_removal_key = ("command.obj", 0x6C070)
    process_removal_va = 0x00474D80
    process_removal_body = """\
switch (msg.id) {
case MESSAGE_WIDGET:
    switch (msg.codeX) {
    case TCombatWindow::COMBAT_RIGHT_COMMAND_0_ID:
        if (!heroes[currentSide]) {
            NormalDialog(text, 1);
        } else {
            InitiateSpell(ViewSpells(), 0);
            ResetMouse();
        }
        break;
    case TCombatWindow::COMBAT_LEFT_COMMAND_1_ID:
        NormalDialog(text, 2);
        if (gpWindowManager->dialogReturn == DIALOG_RETURN_ACCEPT)
            field_3c = 4;
        ResetMouse();
        break;
    case TCombatWindow::COMBAT_LEFT_COMMAND_0_ID:
        if (DoSurrender()) {
            field_3c = 5;
        }
        ResetMouse();
        break;
    }
    break;
case MESSAGE_MOUSE_MOVE: {
    msgTemp = gpInputManager->PeekEvent();
    int gridIndex = GetGridIndex(mouseX, mouseY);
    UpdateMouseGrid(gridIndex, 0);
    if (!InCombatArea(mouseX, mouseY)) {
        TurnOffHighlighter(1);
        gpWindowManager->ConvertToHover(msg);
        gpMouseManager->SetPointer(6, mouseManager::COMBAT_SET);
        field_132d4 = -1;
        field_132dc = -99;
        return MESSAGE_DISPATCH_CONSUME;
    }
    CombatMessage(field_132e0);
    break;
}
case MESSAGE_KEY_DOWN:
    switch (msg.codeX) {
    case KEYCODE_F5:
        WritePrefs();
        break;
    case KEYCODE_F6:
        SetCombatGrid(1, 0, 0, 1);
        break;
    case KEYCODE_F7:
        SetCombatGrid(0, 1, 0, 1);
        break;
    case KEYCODE_F8:
        SetCombatGrid(0, 0, 1, 1);
        break;
    case KEYCODE_KP_MINUS:
        combatWindow->scroll_rollover(-1);
        break;
    case KEYCODE_KP_2:
        combatWindow->scroll_rollover(1);
        break;
    case KEYCODE_F:
        if (currentArmy->creatureType == CREATURE_FAERIE_DRAGON)
            InitiateSpell(currentArmy->field_4e0, 1);
        break;
    case KEYCODE_T:
        ViewArmy(get_current_army(), 0);
        ResetMouse();
        break;
    }
    break;
}
"""
    process_removed, process_descriptions = \
        retail_proven_dc_only_removed_helpers(
            process_removal_key, process_removal_body, process_removal_va)
    expected_process_removed = frozenset((
        "combatManager::FullUpdate", "combatManager::InitMouse",
        "combatManager::MoveCursorMenu", "combatManager::MoveCursorTo",
        "combatManager::ScrollCombatArea"))
    if process_removed != expected_process_removed \
            or len(process_descriptions) != 2:
        failures.append(
            "retail-byte-proved ProcessCombatMsg removals did not classify")
    if contract_violations(process_removal_body, process_removal_key):
        failures.append("Complete ProcessCombatMsg replacement groups failed")
    process_order_helpers, process_order_descriptions = \
        retail_proven_dc_only_order_helpers(
            process_removal_key, process_removal_body, process_removal_va)
    if process_order_helpers != frozenset((
            "heroWindowManager::ConvertToHover",
            "mouseManager::SetPointer")) \
            or len(process_order_descriptions) != 1:
        failures.append(
            "retail-byte-proved ProcessCombatMsg order did not classify")
    duplicate_hover_body = process_removal_body.replace(
        "TurnOffHighlighter(1);",
        "gpWindowManager->ConvertToHover(msg); TurnOffHighlighter(1);")
    if retail_proven_dc_only_order_helpers(
            process_removal_key, duplicate_hover_body,
            process_removal_va)[0]:
        failures.append(
            "duplicate ProcessCombatMsg hover group classified as skew")
    if not contract_violations(duplicate_hover_body, process_removal_key):
        failures.append("duplicate ProcessCombatMsg hover group passed")
    obsolete_controller_body = process_removal_body.replace(
        "WritePrefs();", "ScrollCombatArea(1); WritePrefs();")
    obsolete_controller_removed = retail_proven_dc_only_removed_helpers(
        process_removal_key, obsolete_controller_body,
        process_removal_va)[0]
    if obsolete_controller_removed & frozenset((
            "combatManager::InitMouse", "combatManager::MoveCursorMenu",
            "combatManager::MoveCursorTo",
            "combatManager::ScrollCombatArea")):
        failures.append(
            "restored Dreamcast ProcessCombatMsg controller call classified")
    if not contract_violations(
            obsolete_controller_body, process_removal_key):
        failures.append(
            "restored Dreamcast ProcessCombatMsg controller call passed")
    surrender_removal_key = ("command.obj", 0x6E990)
    surrender_removal_body = """\
gSurrenderCost695030 = get_surrender_cost();
sprintf(gText, gpGeneralText->GetText(33),
        heroes[1 - currentSide]->name, gSurrenderCost695030);
NormalDialog(gText, 2, -1, 0);
return gpWindowManager->dialogReturn == DIALOG_RETURN_ACCEPT;
"""
    surrender_removed, surrender_descriptions = \
        retail_proven_dc_only_removed_helpers(
            surrender_removal_key, surrender_removal_body, None)
    if surrender_removed != frozenset(("combatManager::FullUpdate",)) \
            or len(surrender_descriptions) != 1:
        failures.append(
            "inlined retail-byte-proved DoSurrender removal did not classify")
    if retail_proven_dc_only_removed_helpers(
            retail_removal_key, retail_removal_body, None)[0]:
        failures.append(
            "ordinary retail removal admitted a VA-less helper body")
    obsolete_surrender_body = surrender_removal_body.replace(
        "return gpWindowManager", "FullUpdate(); return gpWindowManager")
    if retail_proven_dc_only_removed_helpers(
            surrender_removal_key, obsolete_surrender_body, None)[0]:
        failures.append(
            "restored Dreamcast DoSurrender FullUpdate classified out")
    if not contract_violations(
            obsolete_surrender_body, surrender_removal_key):
        failures.append("restored Dreamcast DoSurrender FullUpdate passed")
    flattened_trigger_body = retail_removal_body.replace(
        "hero_data->location = heroObject->get_trigger();",
        "heroObject->FindTrigger(x, y); hero_data->location.x = x;")
    if not contract_violations(flattened_trigger_body, retail_removal_key):
        failures.append("flattened readHeroData get_trigger tail passed")
    fake_shrink_helper_body = retail_removal_body.replace(
        "int x;", "int x; readHeroArmies(infile, hero_data, mapVersion);")
    if not contract_violations(
            fake_shrink_helper_body, retail_removal_key):
        failures.append("invented readHeroData caller-shrink helper passed")
    split_artifact_counter_body = retail_removal_body.replace(
        "for (x = 0; x < hero_data->numInBackpack; ++x)",
        "for (int carried = 0; carried < hero_data->numInBackpack; "
        "++carried)")
    if not contract_violations(
            split_artifact_counter_body, retail_removal_key):
        failures.append("split readHeroData artifact counters passed")
    block_buffer_body = retail_removal_body.replace(
        "int int_buffer;", "").replace(
        "count = 18;", "int int_buffer;\ncount = 18;")
    if not contract_violations(block_buffer_body, retail_removal_key):
        failures.append("block-local readHeroData buffer passed")
    move_hero_key = ("philai.obj", 0x10E9A8)
    move_hero_body = """\
long max_distance = 1000;
max_distance = max(
    current_hero->movePoints
        + static_cast<unsigned short>(current_hero->field_041) + 200,
    1000);
"""
    if contract_violations(move_hero_body, move_hero_key):
        failures.append("retail-proved move_hero max floor did not pass")
    move_hero_mutations = (
        move_hero_body.replace("max(", "min("),
        move_hero_body.replace("max(", "identity("),
        move_hero_body.replace("unsigned short", "short"),
        move_hero_body.replace("1000);", "999);"),
    )
    for mutation in move_hero_mutations:
        if not contract_violations(mutation, move_hero_key):
            failures.append("broken retail-proved move_hero max floor passed")
            break
    update_slots_key = ("swapmgr.obj", 0x15CDBC)
    update_slots_probe = """\
for (int iHero = 0; iHero < 2; ++iHero)
    for (int slot = const_first_artifact_slot;
         slot < kNumArtifactSlots + 1;
         slot++)
        UpdateSlot(iHero, static_cast<TArtifactSlot>(slot));
"""
    if contract_violations(update_slots_probe, update_slots_key):
        failures.append("aligned update_all_slots helper failed source gate")
    if not contract_violations(
            update_slots_probe.replace(
                "UpdateSlot(iHero, static_cast<TArtifactSlot>(slot));", ""),
            update_slots_key):
        failures.append("update_all_slots without UpdateSlot passed source gate")
    update_key = ("swapmgr.obj", 0x15EA00)
    update_probe = """\
message msg;
int value = heroes[side]->GetPrimarySkill(i);
parent->BroadcastMessage(&msg);
update_all_slots();
"""
    if contract_violations(update_probe, update_key):
        failures.append("aligned swapManager::Update failed source gate")
    flattened_update = update_probe.replace(
        "update_all_slots();", update_slots_probe)
    if not contract_violations(flattened_update, update_key):
        failures.append("Update with flattened update_all_slots helper passed")
    reordered_update = """\
update_all_slots();
message msg;
int value = heroes[side]->GetPrimarySkill(i);
parent->BroadcastMessage(&msg);
"""
    if not contract_violations(reordered_update, update_key):
        failures.append("Update with reordered trailing helper passed source gate")
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
    setup_order_key = ("singleselectionwindow.obj", 0x135AA4)
    setup_order_va = PROVEN_ORDER_SKEWS[setup_order_key][0].caller_va
    setup_complete_order = """\
gpGame->SetupOrigData();
if (IsHost()) {
    GetHeaders(&HeadersA);
    scenarioOptionsStarted = 1;
    HighlightFile(defaultMapFileName);
    SetCurrentMap(currentMap, false);
}
"""
    setup_helpers, setup_descriptions = proven_dc_only_order_helpers(
        setup_order_key, setup_complete_order, setup_order_va,
        {setup_order_va})
    if setup_helpers != frozenset(
            PROVEN_ORDER_SKEWS[setup_order_key][0].dc_only_helpers) \
            or len(setup_descriptions) != 1:
        failures.append("exact SetupNewGameMode order skew did not classify")
    if proven_dc_only_order_helpers(
            setup_order_key, setup_complete_order, setup_order_va, set())[0]:
        failures.append("non-exact SetupNewGameMode order skew classified")
    setup_erased = setup_complete_order.replace(
        "    GetHeaders(&HeadersA);\n", "")
    if proven_dc_only_order_helpers(
            setup_order_key, setup_erased, setup_order_va,
            {setup_order_va})[0]:
        failures.append("erased SetupNewGameMode host pass classified")
    setup_groups = (
        CallGroup(2747, ("game::SetupOrigData",)),
        CallGroup(2789, ("TSingleSelectionWindow::GetHeaders",)),
        CallGroup(2792, ("TSingleSelectionWindow::HighlightFile",)),
        CallGroup(2793, ("TSingleSelectionWindow::SetCurrentMap",)),
        CallGroup(2796, ("TSingleSelectionWindow::ShowWidget",)),
    )
    if groups_without_helpers(setup_groups, setup_helpers) != (
            CallGroup(2747, ("game::SetupOrigData",)),
            CallGroup(2796, ("TSingleSelectionWindow::ShowWidget",)),):
        failures.append("SetupNewGameMode order filter erased another call")
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
    obscured_object_key = ("mapcell.obj", 0xF4A9C)
    obscured_object_probe = """\
if (valid)
    return obscuredType;
return NOTHING;
"""
    if contract_violations(obscured_object_probe, obscured_object_key):
        failures.append(
            "aligned get_obscured_object guarded-return shape did not pass")
    flattened_obscured_object = "return valid ? obscuredType : NOTHING;\n"
    if not contract_violations(flattened_obscured_object,
                               obscured_object_key):
        failures.append(
            "flattened get_obscured_object guarded-return shape passed")
    object_cell_key = ("mapcell.obj", 0xEBF6C)
    object_cell_probe = "return &gpGame->worldMap.objects[objectIndex];\n"
    if contract_violations(object_cell_probe, object_cell_key):
        failures.append("aligned TObjectCell::get_object body did not pass")
    flattened_object_cell = (
        "return gpGame->worldMap.objects.begin() + objectIndex;\n")
    if not contract_violations(flattened_object_cell, object_cell_key):
        failures.append("flattened TObjectCell::get_object body passed")
    trigger_cell_key = ("mapcell.obj", 0xEBF98)
    trigger_cell_probe = """\
if (is_trigger)
    return this;
if (type == NOTHING || type == ANCHOR_POINT || type == EVENT
    || type == HOLY_GRAIL)
    return 0;
if (object_type_index < 0
    || object_type_index >= gpGame->worldMap.objects.size())
    return 0;
CObject* object = &gpGame->worldMap.objects[object_type_index];
type_point location = object->get_trigger();
if (location.x < 0)
    return 0;
return gpGame->get_cell(location);
"""
    if contract_violations(trigger_cell_probe, trigger_cell_key):
        failures.append("aligned get_trigger_cell source shape did not pass")
    indexed_trigger_cell = trigger_cell_probe.replace(
        "if (object_type_index < 0",
        "short index = object_type_index;\nif (index < 0").replace(
            "object_type_index >=", "index >=").replace(
                "objects[object_type_index]", "objects[index]")
    flattened_trigger_cell = trigger_cell_probe.replace(
        "type_point location = object->get_trigger();",
        "int x; int y; object->FindTrigger(x, y);\n"
        "type_point location(x, y, object->z);")
    flattened_trigger_lookup = trigger_cell_probe.replace(
        "return gpGame->get_cell(location);",
        "return gpGame->worldMap.cell(location);")
    if any(not contract_violations(probe, trigger_cell_key) for probe in (
            indexed_trigger_cell,
            flattened_trigger_cell,
            flattened_trigger_lookup)):
        failures.append("flattened get_trigger_cell source shape passed")
    object_trigger_key = ("mapcell.obj", 0xEDA4C)
    object_trigger_probe = """\
int result_x;
int result_y;
FindTrigger(result_x, result_y);
return type_point(result_x, result_y, z);
"""
    if contract_violations(object_trigger_probe, object_trigger_key):
        failures.append("aligned CObject::get_trigger body did not pass")
    pointer_trigger_call = object_trigger_probe.replace(
        "FindTrigger(result_x, result_y);",
        "FindTrigger(&result_x, &result_y);")
    flattened_trigger_ctor = object_trigger_probe.replace(
        "return type_point(result_x, result_y, z);",
        "type_point result; result.x = result_x; result.y = result_y; "
        "result.z = z; return result;")
    if any(not contract_violations(probe, object_trigger_key) for probe in (
            pointer_trigger_call, flattened_trigger_ctor)):
        failures.append("flattened CObject::get_trigger body passed")
    find_trigger_key = ("mapcell.obj", 0xEDAB8)
    find_trigger_probe = """\
result_x = -1;
result_y = -1;
CObjectType* ObjType = &gpGame->worldMap.objectTypes[typeIndex];
for (int Vert = 0; Vert < ObjType->height; ++Vert) {
    if (y - Vert < 0 || y - Vert >= gMapHeight)
        continue;
    for (int Horiz = 0; Horiz < ObjType->width; ++Horiz) {
        if (x - Horiz < 0 || x - Horiz >= gMapWidth)
            continue;
        if (ObjType->triggerCells[47 - Vert * 8 - Horiz]) {
            result_x = x - Horiz;
            result_y = y - Vert;
            return;
        }
    }
}
"""
    if contract_violations(find_trigger_probe, find_trigger_key):
        failures.append("aligned CObject::FindTrigger body did not pass")
    pointer_find_trigger = find_trigger_probe.replace(
        "result_x = -1;", "*result_x = -1;").replace(
            "result_y = -1;", "*result_y = -1;")
    if not contract_violations(pointer_find_trigger, find_trigger_key):
        failures.append("pointer-form CObject::FindTrigger body passed")
    object_trigger_header_probe = """\
type_point get_trigger() const;
void FindTrigger(int& resultX, int& resultY) const;
"""
    if cobject_trigger_header_violations(object_trigger_header_probe):
        failures.append("aligned CObject trigger declarations did not pass")
    pointer_trigger_header = object_trigger_header_probe.replace(
        "int& resultX, int& resultY", "int* resultX, int* resultY")
    mutable_trigger_header = object_trigger_header_probe.replace(
        ") const;", ");")
    if any(not cobject_trigger_header_violations(probe) for probe in (
            pointer_trigger_header, mutable_trigger_header)):
        failures.append("wrong CObject trigger declarations passed")
    get_map_object_key = ("mapcell.obj", 0xEC098)
    get_map_object_probe = """\
if (type == HERO) {
    hero* current_hero = gpGame->GetHero(extraInfo);
    return current_hero->get_obscured_object();
}
if (type == BOAT) {
    boat* current_boat = gpGame->GetBoat(extraInfo);
    return current_boat->get_obscured_object();
}
return type;
"""
    if contract_violations(get_map_object_probe, get_map_object_key):
        failures.append("aligned get_map_object helper shape did not pass")
    flattened_get_map_object = get_map_object_probe.replace(
        "return current_hero->get_obscured_object();",
        "return current_hero->valid ? current_hero->obscuredType : NOTHING;")
    if not contract_violations(flattened_get_map_object,
                               get_map_object_key):
        failures.append("flattened get_map_object obscurer helper passed")
    reordered_get_map_object = get_map_object_probe.replace(
        "if (type == HERO)", "if (type == HERO_PLACEHOLDER)").replace(
            "if (type == BOAT)", "if (type == HERO)").replace(
            "if (type == HERO_PLACEHOLDER)", "if (type == BOAT)")
    if not contract_violations(reordered_get_map_object,
                               get_map_object_key):
        failures.append("reordered get_map_object hero/boat arms passed")
    is_diggable_key = ("mapcell.obj", 0xEC254)
    is_diggable_probe = """\
if (GroundSet == eTerrainWater || GroundSet == eTerrainRock)
    return 0;
if (!(flags_00_11 & 0x40))
    return 0;

TAdventureObjectType object_type = get_map_object();
if (object_type != ANCHOR_POINT) {
    if (object_type != HOLY_GRAIL && object_type != NOTHING)
        return 0;
} else {
    for (long i = 0; i < objects.size(); ++i) {
        if (objects[i].get_object()->get_type() == TERRAIN_HOLE)
            return 0;
    }
}
return 1;
"""
    if contract_violations(is_diggable_probe, is_diggable_key):
        failures.append("aligned is_diggable source shape did not pass")
    combined_diggable_guards = is_diggable_probe.replace(
        "if (GroundSet == eTerrainWater || GroundSet == eTerrainRock)\n"
        "    return 0;\nif (!(flags_00_11 & 0x40))",
        "if (GroundSet == eTerrainWater || GroundSet == eTerrainRock\n"
        "    || !(flags_00_11 & 0x40))")
    flattened_diggable_type = is_diggable_probe.replace(
        "TAdventureObjectType object_type = get_map_object();",
        "TAdventureObjectType object_type = type;")
    unsigned_diggable_loop = is_diggable_probe.replace(
        "for (long i = 0;", "for (unsigned int i = 0;")
    flattened_diggable_chain = is_diggable_probe.replace(
        "objects[i].get_object()->get_type()",
        "gpGame->worldMap.objectTypes[gpGame->worldMap.objects["
        "objects[i].objectIndex].typeIndex].objectType")
    broken_diggable_probes = (
        combined_diggable_guards,
        flattened_diggable_type,
        unsigned_diggable_loop,
        flattened_diggable_chain,
    )
    if any(not contract_violations(probe, is_diggable_key)
           for probe in broken_diggable_probes):
        failures.append("broken is_diggable Dreamcast source shape passed")
    obscured_trigger_key = ("findpath.obj", 0xA113C)
    obscured_trigger_probe = "return was_trigger;\n"
    if contract_violations(obscured_trigger_probe, obscured_trigger_key):
        failures.append("aligned obscured_is_trigger body did not pass")
    if not contract_violations("return was_trigger != 0;\n",
                               obscured_trigger_key):
        failures.append("flattened obscured_is_trigger accessor passed")
    special_terrain_key = ("mapcell.obj", 0xEC3B4)
    special_terrain_probe = """\
if (type == HERO && (cellFlags & 0x1000)) {
    hero* our_hero = gpGame->GetHero(extraInfo);
    if (our_hero->get_obscured_object() == GARRISON
            && our_hero->obscured_is_trigger()
            && objectIndex == 1)
        return GARRISON;
}
if (type == GARRISON
        && (cellFlags & 0x1000)
        && objectIndex == 1)
    return type;
for (int i = objects.size(); i-- > 0;) {
    CObject* object = objects[i].get_object();
    CObjectType* object_type = object->get_object_type_ptr();
    if (object_type->objectType == CURSED_GROUND
            || object_type->objectType == MAGIC_PLAINS
            || object_type->objectType == HOLY_GROUND
            || object_type->objectType == EVIL_FOG
            || object_type->objectType == CLOVER_FIELD_2
            || object_type->objectType == FAVORABLE_WINDS
            || object_type->objectType == LUCID_POOLS
            || object_type->objectType == FIERY_FIELDS
            || object_type->objectType == ROCKLANDS
            || object_type->objectType == MAGIC_CLOUDS)
        return object_type->objectType;
}
return NOTHING;
"""
    if contract_violations(special_terrain_probe, special_terrain_key):
        failures.append("aligned get_special_terrain source shape did not pass")
    flattened_special_obscurer = special_terrain_probe.replace(
        "our_hero->get_obscured_object() == GARRISON",
        "our_hero->valid && our_hero->obscuredType == GARRISON")
    flattened_special_trigger = special_terrain_probe.replace(
        "our_hero->obscured_is_trigger()", "our_hero->was_trigger")
    cached_special_roots = special_terrain_probe.replace(
        "if (type == HERO", "TAdventureObjectType cellType = type;\n"
        "game* currentGame = gpGame;\nif (type == HERO")
    unsigned_special_loop = special_terrain_probe.replace(
        "i-- > 0", "i--")
    flattened_special_object = special_terrain_probe.replace(
        "CObject* object = objects[i].get_object();",
        "CObject* object = &gpGame->worldMap.objects["
        "objects[i].objectIndex];")
    shortened_special_answers = special_terrain_probe.replace(
        "            || object_type->objectType == HOLY_GROUND\n", "")
    broken_special_terrain_probes = (
        flattened_special_obscurer,
        flattened_special_trigger,
        cached_special_roots,
        unsigned_special_loop,
        flattened_special_object,
        shortened_special_answers,
    )
    if any(not contract_violations(probe, special_terrain_key)
           for probe in broken_special_terrain_probes):
        failures.append(
            "broken get_special_terrain Dreamcast source shape passed")
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
    assign_data_probe = """\
void AssignData(CMapHeaderData* pData, char* sName, char* sDesc)
{
    static_cast<CMapHeaderData&>(*this) = *pData;
    mapName = sName;
    mapDescription = sDesc;
}
"""
    if game_assign_data_header_violations(assign_data_probe):
        failures.append("aligned Game.h AssignData source shape did not pass")
    broken_assign_data_probes = (
        assign_data_probe.replace("pData", "data"),
        assign_data_probe.replace(
            "mapName = sName;", "mapName.assign(sName, strlen(sName));"),
        assign_data_probe.replace(
            "mapName = sName;\n    mapDescription = sDesc;",
            "mapDescription = sDesc;\n    mapName = sName;"),
        assign_data_probe.replace(
            "    static_cast<CMapHeaderData&>(*this) = *pData;\n", ""),
    )
    if any(not game_assign_data_header_violations(probe)
           for probe in broken_assign_data_probes):
        failures.append("broken Game.h AssignData source shape passed")
    set_next_player_probe = """\
int i = start;
while (i < MAX_PLAYERS) {
    if (humanPlayers[i].IsHuman()) {
        assignedPos = humanPlayers[i].playerPos;
        humanPlayers[i].playerPos = pos;
        humanPlayers[i].heroIndex = -1;
        humanPlayers[i].townIndex = -1;
        return 1;
    }
    ++i;
}
return 1;
"""
    set_next_player_key = ("singleselectionwindow.obj", 0x1304A8)
    if contract_violations(set_next_player_probe, set_next_player_key):
        failures.append(
            "aligned CNetPlayerHandler::SetNextPlayer source shape did not pass")
    broken_set_next_player_probes = (
        set_next_player_probe.replace(
            "humanPlayers[i].IsHuman()", "humanPlayers[i].dpid != 0"),
        set_next_player_probe.replace(
            "while (i < MAX_PLAYERS) {", "while (humanPlayers[i].dpid == 0) {"),
        set_next_player_probe.replace("        return 1;\n    }", "        break;\n    }"),
        set_next_player_probe.replace("    ++i;\n", ""),
    )
    if any(not contract_violations(probe, set_next_player_key)
           for probe in broken_set_next_player_probes):
        failures.append(
            "broken CNetPlayerHandler::SetNextPlayer source shape passed")
    backup_headers_probe = """\
int i;
dest->mapHeader = src->mapHeader;
dest->setup = src->setup;
dest->campaign = src->campaign;
dest->field_1f4d4 = src->field_1f4d4;
dest->difficultyRating = src->difficultyRating;
dest->field_1f635 = src->field_1f635;
if (src == gpGame) {
    saveCurPlayer = gNetLocalGamePos;
} else {
    gNetLocalGamePos = saveCurPlayer;
}
std::copy(src->players, src->players + 8, dest->players);
std::copy(src->heroAvailability,
          src->heroAvailability + sizeof(src->heroAvailability),
          dest->heroAvailability);
std::copy(src->saveFileName,
          src->saveFileName + sizeof(src->saveFileName),
          dest->saveFileName);
std::copy(src->playerDisabled,
          src->playerDisabled + sizeof(src->playerDisabled),
          dest->playerDisabled);
"""
    backup_headers_key = ("singleselectionwindow.obj", 0x12F9C8)
    if contract_violations(backup_headers_probe, backup_headers_key):
        failures.append("aligned BackupGameHeaders source shape did not pass")
    broken_backup_headers_probes = (
        backup_headers_probe.replace("int i;\n", ""),
        backup_headers_probe.replace(
            "dest->mapHeader = src->mapHeader;\n"
            "dest->setup = src->setup;",
            "dest->setup = src->setup;\n"
            "dest->mapHeader = src->mapHeader;"),
        backup_headers_probe.replace(
            "std::copy(src->players, src->players + 8, dest->players);",
            "for (i = 0; i < 8; i++) {\n"
            "    dest->players[i] = src->players[i];\n}"),
        backup_headers_probe.replace(
            "if (src == gpGame) {\n"
            "    saveCurPlayer = gNetLocalGamePos;\n"
            "} else {\n"
            "    gNetLocalGamePos = saveCurPlayer;\n}",
            "if (src == gpGame)\n"
            "    saveCurPlayer = gNetLocalGamePos;\n"
            "else\n"
            "    gNetLocalGamePos = saveCurPlayer;"),
        backup_headers_probe.replace(
            "std::copy(src->saveFileName,\n"
            "          src->saveFileName + sizeof(src->saveFileName),\n"
            "          dest->saveFileName);\n",
            ""),
    )
    if any(not contract_violations(probe, backup_headers_key)
           for probe in broken_backup_headers_probes):
        failures.append("broken BackupGameHeaders source shape passed")
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
    update_task_header_probe = """\
class __declspec(novtable) CNewPlayerUpdateTask {
public:
    virtual void Go() = 0;
    virtual void Tick() = 0;
    virtual void Finish() = 0;
    ~CNewPlayerUpdateTask();
    unsigned long m_dpid;
    int m_nextHeader;
    std::vector<SHeaderRequest> m_requests;
    unsigned long m_lastSendTime;
    unsigned char m_finished;
};
class CNewPlayerUpdateProc : public CNewPlayerUpdateTask {
public:
    CNewPlayerUpdateProc(unsigned long dpid) {
        m_dpid = dpid;
        m_nextHeader = 0;
        m_finished = 0;
        m_lastSendTime = 0;
    }
    virtual void Go();
    virtual void Tick();
    virtual void Finish();
    void RequestConfirmation();
    void HandleRequests();
};
class t_map_list_update : public CNewPlayerUpdateProc {
public:
    t_map_list_update(unsigned long dpid);
    virtual void Go();
    virtual void Tick();
    virtual void Finish();
};
class CNewPlayerUpdateMan {
public:
    CNewPlayerUpdateTask* m_procs[8];
};
"""
    update_task_source_probe = """\
VA(0x00589240, 0x2F)
t_map_list_update::t_map_list_update(unsigned long dpid)
    : CNewPlayerUpdateProc(dpid)
{
}
CNewPlayerUpdateTask::~CNewPlayerUpdateTask()
{
}
inline void CNewPlayerUpdateProc::RequestConfirmation()
{
    logFile.Log(DATA_COMPGEN(0x006834d0, requestingConfirmLog,
                            "Requesting confirmation: %d"));
    CReqHeaderConfirmMsg msg;
    TransmitRemoteDataDPID(&msg, m_dpid, false, true);
}
VA(0x00578010, 0x272)
inline void CNewPlayerUpdateProc::HandleRequests()
{
}
"""
    if new_player_update_contract_violations(
            update_task_header_probe, update_task_source_probe):
        failures.append("aligned NewPlayer update type chain did not pass")
    broken_update_task_probes = (
        (update_task_header_probe.replace("__declspec(novtable) ", ""),
         update_task_source_probe),
        (update_task_header_probe.replace(
            "class t_map_list_update : public CNewPlayerUpdateProc",
            "class t_map_list_update : public CNewPlayerUpdateTask"),
         update_task_source_probe),
        (update_task_header_probe.replace(
            "CNewPlayerUpdateTask* m_procs[8]",
            "CNewPlayerUpdateProc* m_procs[8]"),
         update_task_source_probe),
        (update_task_header_probe, update_task_source_probe.replace(
            ": CNewPlayerUpdateProc(dpid)",
            ": CNewPlayerUpdateTask()")),
        (update_task_header_probe, update_task_source_probe.replace(
            "CNewPlayerUpdateTask::~CNewPlayerUpdateTask()",
            "CNewPlayerUpdateProc::~CNewPlayerUpdateProc()")),
        (update_task_header_probe.replace(
            "    void HandleRequests();\n};\nclass t_map_list_update",
            "};\nclass t_map_list_update").replace(
                "    virtual void Finish();\n};\nclass CNewPlayerUpdateMan",
                "    virtual void Finish();\n"
                "    void HandleRequests();\n};\n"
                "class CNewPlayerUpdateMan"),
         update_task_source_probe),
        (update_task_header_probe, update_task_source_probe.replace(
            '"Requesting confirmation: %d"));',
            '"Requesting confirmation: %d"), m_dpid);')),
        (update_task_header_probe, update_task_source_probe.replace(
            "CNewPlayerUpdateProc::HandleRequests()",
            "t_map_list_update::HandleRequests()")),
    )
    if any(not new_player_update_contract_violations(header, source)
           for header, source in broken_update_task_probes):
        failures.append("broken NewPlayer update type chain passed")
    update_msg_header_probe = """\
class CGameHeaderInfoInitMsg : public CNetMsg {
public:
    unsigned long m_numMaps;
    unsigned char m_netGame;
    CGameHeaderInfoInitMsg(unsigned long numMaps,
                           unsigned char loadGameMode,
                           unsigned long msgSize)
        : CNetMsg(RS_GAME_HEADER_INFO_INIT, msgSize)
    {
        m_numMaps = numMaps;
        m_netGame = loadGameMode;
    }
};
class CGameHeaderInfoInitMsgEx : public CGameHeaderInfoInitMsg {
public:
    char m_version[20];
    CGameHeaderInfoInitMsgEx(const char* version, unsigned long numMaps,
                             unsigned char loadGameMode)
        : CGameHeaderInfoInitMsg(numMaps, loadGameMode,
                                 sizeof(CGameHeaderInfoInitMsgEx))
    {
        memset(m_version, 0, sizeof(m_version));
        strncpy(m_version, version, sizeof(m_version) - 1);
    }
};
"""
    update_msg_source_probe = """\
VA(0x005789F0, 0x9E)
void CNewPlayerUpdateProc::Go()
{
    CGameHeaderInfoInitMsgEx initMsg(
        gUnnamed69fbe8->gameVersion,
        gUnnamed69fbe8->HeadersA.size(),
        gUnnamed69fbe8->m_flag64);
    TransmitRemoteDataDPID(&initMsg, m_dpid, false, true);
}
"""
    if game_header_info_init_contract_violations(
            update_msg_header_probe, update_msg_source_probe):
        failures.append("aligned 1024 header-init message chain did not pass")
    broken_update_msg_probes = (
        (update_msg_header_probe.replace(
            "class CGameHeaderInfoInitMsgEx : public "
            "CGameHeaderInfoInitMsg",
            "class CGameHeaderInfoInitMsgEx : public CNetMsg"),
         update_msg_source_probe),
        (update_msg_header_probe.replace(
            "m_numMaps = numMaps;\n        m_netGame = loadGameMode;",
            "m_netGame = loadGameMode;\n        m_numMaps = numMaps;"),
         update_msg_source_probe),
        (update_msg_header_probe.replace(
            "memset(m_version, 0, sizeof(m_version));\n"
            "        strncpy(m_version, version, sizeof(m_version) - 1);",
            "strncpy(m_version, version, sizeof(m_version) - 1);\n"
            "        memset(m_version, 0, sizeof(m_version));"),
         update_msg_source_probe),
        (update_msg_header_probe,
         update_msg_source_probe.replace("HeadersA", "TransferHeaders")),
        (update_msg_header_probe,
         update_msg_source_probe.replace("false, true", "true, false")),
    )
    if any(not game_header_info_init_contract_violations(header, source)
           for header, source in broken_update_msg_probes):
        failures.append("broken 1024 header-init message chain passed")
    map_file_msg_probe = """\
class CMapFileNameMsg : public CNetMsg {
public:
    unsigned char m_flag;
    int m_number;
    char m_fileName[0x40];
    int m_townTypes[8];
    FILETIME m_fileTime;
    CMapFileNameMsg(unsigned char flag, int number, const char* fileName,
                    int* townTypes, FILETIME fileTime)
        : CNetMsg(RS_MAP_FILE_NAME, sizeof(CMapFileNameMsg))
    {
        m_flag = flag;
        m_number = number;
        strncpy(m_fileName, fileName, 0x3c);
        m_fileTime = fileTime;
        memcpy(m_townTypes, townTypes, sizeof(m_townTypes));
    }
};
"""
    if map_file_name_message_contract_violations(map_file_msg_probe):
        failures.append("aligned CMapFileNameMsg constructor did not pass")
    broken_map_file_msg_probes = (
        map_file_msg_probe.replace("FILETIME m_fileTime;",
                                   "unsigned int m_fileTimeLow;"),
        map_file_msg_probe.replace(
            "const char* fileName,\n                    int* townTypes, "
            "FILETIME fileTime",
            "GameSelectionHeadersStruct* header"),
        map_file_msg_probe.replace(
            "m_number = number;\n        strncpy",
            "strncpy(m_fileName, fileName, 0x3c);\n        m_number = "
            "number;\n        strncpy", 1),
        map_file_msg_probe.replace(
            "m_fileTime = fileTime;\n        memcpy",
            "memcpy(m_townTypes, townTypes, sizeof(m_townTypes));\n"
            "        m_fileTime = fileTime;\n        memcpy", 1),
    )
    if any(not map_file_name_message_contract_violations(probe)
           for probe in broken_map_file_msg_probes):
        failures.append("broken CMapFileNameMsg constructor passed")
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
struct TWallTarget {
    short target_hex;
    short blocked_row;
    short hit_x;
    short hit_y;
    TWallSection wall;
    int get_blocked_hex() const {
        if (blocked_row != -1) return gCastleWallColumns[blocked_row];
        return -1;
    }
};
static const TWallTarget wallTargets[8];
static bool ValidHex(int iHex) {
    return iHex >= 0 && iHex < COMBAT_GRID_CELLS;
}
long get_wall_strength(TWallTargetId target) const {
    return wallStrength[wallTargets[target].wall];
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
    if not combat_manager_header_violations(
            cmbtmgr_inline_probe.replace(
                "wallStrength[wallTargets[target].wall]",
                "wallStrength[target]")):
        failures.append("flattened get_wall_strength header body passed")
    if not combat_manager_header_violations(
            cmbtmgr_inline_probe.replace("TWallSection wall;",
                                         "int wall_id;")):
        failures.append("flattened TWallTarget member names/types passed")
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
    deinlined_attack_direction = roster_text.replace(
        "inline long get_attack_direction(long our_hex, "
        "const army* enemy) const;",
        "long get_attack_direction(long our_hex, const army* enemy) const;",
        1)
    if not any("get_attack_direction" in defect and "inline" in defect
               for _line, defect in army_header_roster_violations(
                   deinlined_attack_direction)):
        failures.append(
            "de-inlined get_attack_direction header contract passed")
    deinlined_get_name = roster_text.replace(
        "    inline const char* GetName() const;",
        "    const char* GetName() const;", 1)
    if not any("GetName" in defect and "inline" in defect
               for _line, defect in army_header_roster_violations(
                   deinlined_get_name)):
        failures.append("de-inlined GetName header contract passed")
    deinlined_get_army_name = roster_text.replace(
        "inline const char* GetArmyName(int type, int count);",
        "const char* GetArmyName(int type, int count);", 1)
    if not any("GetArmyName" in defect and "inline" in defect
               for _line, defect in army_header_roster_violations(
                   deinlined_get_army_name)):
        failures.append("de-inlined GetArmyName header contract passed")

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


def _body_for_claim(masked: str, starts: list[int], claim_line: int,
                    *identities: str):
    """Resolve a VA claim to its canonical active definition when possible.

    Most claims annotate a definition directly, but a TU whose original
    source order differs from retail object order may keep the active body in
    source order and place only an annotated redeclaration in RVA order.  In
    that case the old next-brace rule silently audited the following function.
    A unique name/provenance match is stronger than physical adjacency; retain
    the latter as the fallback for identities the lightweight locator cannot
    decode.
    """
    decoded_identity = False
    for identity in identities:
        if not identity:
            continue
        decoded_identity |= bool(_source.source_names(identity))
        definitions = _definitions_between(
            masked, identity, 0, len(masked))
        if len(definitions) == 1:
            definition = definitions[0]
            return definition.body_open, definition.body_close
    # An exact source identity with no active definition is a claim-only
    # placeholder.  Do not bind it to the next active function merely because
    # that function has the next brace (especially MoveHero vs move_hero).
    if decoded_identity:
        return None
    return _body_after_claim(masked, starts, claim_line)


def _definition_for_owner(masked: str, owner: DefinitionOwner):
    definitions = _definitions_between(masked, owner.name, 0, len(masked))
    return definitions[0] if len(definitions) == 1 else None


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
                     va: int | None, evidence_body: str | None = None) \
            -> CallGroup | None:
        if evidence_body is None:
            evidence_body = body
        ordinary_calls = sum(ref.pool_refs + ref.bsr_calls for ref in refs
                             if _helper_token(ref.name) is not None)
        if ordinary_calls < 2:
            return None
        groups = groups_without_transfers(
            decoded(key, refs).groups,
            lambda callee: transferred(key, callee, body))
        removed, removal_descriptions = proven_dc_only_removed_helpers(
            key, evidence_body, va, exact_vas)
        retail_removed, retail_removal_descriptions = \
            retail_proven_dc_only_removed_helpers(key, evidence_body, va)
        removed = frozenset((*removed, *retail_removed))
        removal_descriptions = (
            *removal_descriptions, *retail_removal_descriptions)
        if removed:
            groups = groups_without_helpers(groups, removed)
            dc_only.update(removal_descriptions)
        helpers, descriptions = proven_dc_only_order_helpers(
            key, evidence_body, va, exact_vas)
        retail_helpers, retail_descriptions = \
            retail_proven_dc_only_order_helpers(key, evidence_body, va)
        helpers = frozenset((*helpers, *retail_helpers))
        descriptions = (*descriptions, *retail_descriptions)
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
        refs = calls.get(key, [])
        if identity is None or not refs and key not in SOURCE_RULES:
            continue
        seen_keys.add(key)
        if claim.path not in cache:
            text = (common.HOMM3_DIR / claim.path).read_text(errors="replace")
            cache[claim.path] = text, _source.mask(text), _line_starts(text)
        text, masked, starts = cache[claim.path]
        row = corpus.by_key.get(key)
        body_source = claim.path
        body_line = claim.line
        owner = PROVEN_DEFINITION_OWNERS.get(key)
        if owner is not None:
            if owner.path not in cache:
                owner_text = (common.HOMM3_DIR / owner.path).read_text(
                    errors="replace")
                cache[owner.path] = (
                    owner_text, _source.mask(owner_text),
                    _line_starts(owner_text))
            text, masked, starts = cache[owner.path]
            definition = _definition_for_owner(masked, owner)
            if definition is None:
                checked += 1
                audited.add(_dc_audit_scope(key))
                missing.append(MissingDefinition(
                    key[0], key[1], owner.path, 1, owner.name))
                continue
            span = definition.body_open, definition.body_close
            body_source = owner.path
            body_line = text.count("\n", 0, definition.head) + 1
        else:
            span = _body_for_claim(
                masked, starts, claim.line, identity[1],
                row.get("name", "") if row is not None else "")
        if span is None:
            continue
        checked += 1
        audited.add(_dc_audit_scope(key))
        body = text[span[0] + 1:span[1]]
        shape_body = apply_proven_call_spellings(
            key, body, claim.va, exact_vas)
        removed, removal_descriptions = proven_dc_only_removed_helpers(
            key, body, claim.va, exact_vas)
        retail_removed, retail_removal_descriptions = \
            retail_proven_dc_only_removed_helpers(key, body, claim.va)
        removed = frozenset((*removed, *retail_removed))
        removal_descriptions = (
            *removal_descriptions, *retail_removal_descriptions)
        dc_only.update(removal_descriptions)
        preliminary = missing_from_body(
            shape_body, [ref.name for ref in refs], key)
        preliminary = [(callee, helper) for callee, helper in preliminary
                       if callee not in removed]
        names = attested(key, refs) if preliminary else set()
        body_missing = False
        for callee, helper in preliminary:
            if callee not in names:
                continue
            if transferred(key, callee, body):
                continue
            body_missing = True
            missing.append(MissingCall(
                claim.va, claim.module, claim.dc_offset, body_source,
                body_line, identity[1], callee, helper))
        if not body_missing and (group := group_defect(
                key, refs, shape_body, claim.va, body)):
            missing.append(MisgroupedCalls(
                claim.va, claim.module, claim.dc_offset, body_source,
                body_line, identity[1], group))
        add_contract_defects(claim.va, key, body_source, body_line,
                             identity[1], body)

    # /Ob2 can fold every copy of a restored helper, leaving no retail VA to
    # claim.  Its Dreamcast provenance line still identifies an active source
    # definition whose own helper graph must remain intact.  Without this
    # second pass, flattening CheckLuck's get_controller call would pass just
    # because do_attack continued to spell CheckLuck.
    provenance_rows: dict[tuple[str, str, int], list[dict[str, str]]] = {}
    for row in corpus.functions:
        key = corpus.key(row)
        if key in seen_keys \
                or key not in calls and key not in SOURCE_RULES:
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
                refs = calls.get(key, [])
                removed, removal_descriptions = \
                    retail_proven_dc_only_removed_helpers(key, body, None)
                dc_only.update(removal_descriptions)
                preliminary = missing_from_body(
                    body, [ref.name for ref in refs], key)
                preliminary = [
                    (callee, helper) for callee, helper in preliminary
                    if callee not in removed]
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

    object_header = common.HOMM3_DIR / "include/advmgr_objects.h"
    object_header_text = object_header.read_text(errors="replace")
    object_header_relpath = "include/advmgr_objects.h"
    audited.add(_file_audit_scope(object_header_relpath))
    object_trigger_defects = cobject_trigger_header_violations(
        object_header_text)
    checked += 1
    missing.extend(FileContractViolation(object_header_relpath, line,
                                         description)
                   for line, description in object_trigger_defects)

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
    assign_data_defects = game_assign_data_header_violations(game_text)
    computer_team_defects = game_is_computer_team_header_violations(game_text)
    randomize_header_defects = game_randomize_header_violations(game_text)
    checked += 4
    missing.extend(FileContractViolation("include/game.h", line,
                                         description)
                   for line, description in
                   get_hero_defects + assign_data_defects
                   + computer_team_defects
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
    selection_defects.extend(new_player_update_contract_violations(
        selection_text, selection_source_text))
    selection_defects.extend(game_header_info_init_contract_violations(
        selection_text, selection_source_text))
    selection_defects.extend(map_file_name_message_contract_violations(
        selection_text))
    selection_defects.extend(finish_message_constructor_contract_violations(
        selection_source_text))
    checked += 9
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
        if key not in calls and key not in SOURCE_RULES:
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
                refs = calls.get(key, [])
                names = attested(key, refs)
                enforced = [(callee, helper)
                            for callee in names
                            if (helper := _helper_token(callee)) is not None]
                if not enforced and key not in SOURCE_RULES:
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
