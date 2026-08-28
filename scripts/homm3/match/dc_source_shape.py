#!/usr/bin/env python3
"""Fatal gate for Dreamcast-attested helper shape in reconstructed functions.

Retail bytes remain authoritative, but an exact x86 lowering is not sufficient
when the Dreamcast CodeView call graph proves that the original source used a
named helper.  For every function carrying a ``dc 0x...`` source claim, and
every active unclaimed definition retaining its ``E:\\gamedcs`` provenance
line, this gate checks that each source-visible game helper from
``evidence/dc-xref-graph.tsv`` is still named in the reconstructed C++ body.
The only exception is a proof-carrying Complete transfer: both the old caller's
forwarding shape and an exact retail receiver must retain the operation.  This
is stricter than a waiver and records an independently proved cross-function
source change.
Provenance-marked header definitions are audited by original source file and
line rather than by emitting object, because `/Ob2` may inline every retail
copy and because one header body can be shared by several TUs.

There is no score threshold or score-based waiver: a local byte maximum cannot
override an attested source fact.  The embedded negative controls cover the
SetupHeroView defect that caused this gate to land, reordered helpers inside
one CodeView group, and flattening two distinct breakpoint groups together.
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


@dataclass(frozen=True)
class CallTransfer:
    description: str
    receiver_path: str
    receiver_name: str
    receiver_va: int
    caller_pattern: str
    receiver_pattern: str


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


# Named calls cover most recoverable shape automatically. These bounded
# contracts preserve source-visible facts that disappear before the SH4 xref
# graph: inlined accessors/operators, a source order hidden by scheduling, and
# nesting within a single attested statement group.
SOURCE_RULES: dict[tuple[str, int], tuple[SourceRule, ...]] = {
    ("game.obj", 0xA3E5C): (
        SourceRule(
            "LoadMinePool retains Dreamcast's signed int x local; an exact "
            "unsigned spelling is byte proof but not source-shape closure",
            r"(?m)^[ \t]*int[ \t]+x[ \t]*;"),
    ),
    ("philai.obj", 0x10FEB8): (
        SourceRule(
            "value_of_experience keeps Dreamcast's no-argument const hero "
            "accessor at line 1718",
            r"\bint\s+increment\s*=\s*current_hero\s*->\s*"
            r"GetExperienceIncrement\s*\(\s*\)\s*;"),
        SourceRule(
            "value_of_experience keeps the line-1719 army conversion as a "
            "separate statement before the line-1721 return",
            r"\bfloat\s+army_value\s*=\s*float\s*\(\s*current_army\s*->\s*"
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
            r"value_of_experience\s*\(\s*our_hero\s*,\s*&\s*"
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


def missing_from_body(body: str, callees: list[str]) -> list[tuple[str, str]]:
    """Return ``[(callee, helper)]`` not named in one active source body."""
    active = _source.mask(body)
    missing = []
    for callee in sorted(set(callees)):
        helper = _helper_token(callee)
        if helper is None:
            continue
        if not re.search(r"\b" + re.escape(helper) + r"\s*\(", active):
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
    token order.  Distinct breakpoint groups must map to distinct, increasing
    chunks.
    """
    chunks = _statement_chunks(_source.mask(body))
    cursor = 0
    for group in groups:
        helpers = [helper for callee in group.callees
                   if (helper := _helper_token(callee)) is not None]
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
        cursor = last + 1
    return None


def contract_violations(body: str, key: tuple[str, int]) -> list[SourceRule]:
    """Selected no-call/order/nesting facts not expressible by DC xrefs."""
    active = _source.mask(body)
    defects = []
    for rule in SOURCE_RULES.get(key, ()):
        count = len(re.findall(rule.pattern, active, re.DOTALL))
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
    attested = ["hero::HasSecondarySkill"]
    direct = "if (currentHero->skillOrder[eSecSkillBattleTactics] > 0) {}"
    if missing_from_body(direct, attested) != [
            ("hero::HasSecondarySkill", "HasSecondarySkill")]:
        failures.append("direct skillOrder test hid missing HasSecondarySkill")
    aligned = "if (currentHero->HasSecondarySkill(eSecSkillBattleTactics)) {}"
    if missing_from_body(aligned, attested):
        failures.append("attested HasSecondarySkill call did not pass")
    commented = "// currentHero->HasSecondarySkill(0);\nif (flag) {}"
    if not missing_from_body(commented, attested):
        failures.append("commented helper call was treated as source shape")
    if missing_from_body("Foo value;", ["Foo::Foo", "Foo::~Foo",
                                         "Foo::operator[]"]):
        failures.append("implicit constructor/destructor/operator was enforced")
    if missing_from_body("", ["std::vector<int>::_M_insert_overflow"]):
        failures.append("compiler/library implementation helper was enforced")
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
    experience_key = ("philai.obj", 0x10FEB8)
    experience_probe = """\
int increment = current_hero->GetExperienceIncrement();
float army_value = float(current_army->get_AI_value());
return (float(gHeroGoldCost) + army_value) / float(increment * 40);
"""
    if contract_violations(experience_probe, experience_key):
        failures.append("aligned value_of_experience contract did not pass")
    flattened_experience = experience_probe.replace(
        "float army_value = float(current_army->get_AI_value());\n", "").replace(
            "army_value", "float(current_army->get_AI_value())")
    if not any("separate statement" in rule.description for rule in
               contract_violations(flattened_experience, experience_key)):
        failures.append("flattened value_of_experience conversion passed")
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
    hero_bonuses_key = ("philai.obj", 0x10FEF4)
    hero_bonuses_probe = """\
type_spellvalue caster(our_hero);
our_hero->turnExperienceToRVRatio =
    value_of_experience(our_hero, &our_hero->army);
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
                  ContractViolation | FileContractViolation]]:
    """Return ``(DC source definitions audited, source-shape defects)``."""
    current = _current_functions()
    exact_vas = _current_exact_vas()
    calls = _xref_calls()
    corpus = dreamcast.Corpus()
    checked = 0
    missing: list[MissingCall | MissingDefinition | MisgroupedCalls |
                  ContractViolation | FileContractViolation] = []
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

    def group_defect(key: tuple[str, int], refs: list[XrefCall], body: str) \
            -> CallGroup | None:
        ordinary_calls = sum(ref.pool_refs + ref.bsr_calls for ref in refs
                             if _helper_token(ref.name) is not None)
        if ordinary_calls < 2:
            return None
        groups = groups_without_transfers(
            decoded(key, refs).groups,
            lambda callee: transferred(key, callee, body))
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
        body = text[span[0] + 1:span[1]]
        preliminary = missing_from_body(body, [ref.name for ref in refs])
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
        if not body_missing and (group := group_defect(key, refs, body)):
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
                definition = definitions[0]
                body = text[definition.body_open + 1:definition.body_close]
                refs = calls[key]
                preliminary = missing_from_body(
                    body, [ref.name for ref in refs])
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
                        key, refs, body)):
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
    cmbtmgr_defects = combat_manager_header_violations(cmbtmgr_text)
    checked += 1
    missing.extend(FileContractViolation("include/cmbtmgr.h", line,
                                         description)
                   for line, description in cmbtmgr_defects)

    struct_header = common.HOMM3_DIR / "include/struct.h"
    struct_text = struct_header.read_text(errors="replace")
    type_point_defects = type_point_header_violations(struct_text)
    checked += 1
    missing.extend(FileContractViolation("include/struct.h", line,
                                         description)
                   for line, description in type_point_defects)

    split_header = common.HOMM3_DIR / "include/armygrp_split.h"
    split_text = split_header.read_text(errors="replace")
    split_defects = split_window_header_violations(split_text)
    checked += 1
    missing.extend(FileContractViolation("include/armygrp_split.h", line,
                                         description)
                   for line, description in split_defects)

    window_header = common.HOMM3_DIR / "include/window.h"
    window_text = window_header.read_text(errors="replace")
    window_defects = window_header_violations(window_text)
    checked += 1
    missing.extend(FileContractViolation("include/window.h", line,
                                         description)
                   for line, description in window_defects)

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
                        body, [callee for callee, _ in enforced]):
                    body_missing = True
                    missing.append(MissingCall(
                        None, key[0], key[1], relpath,
                        text.count("\n", 0, definition.head) + 1,
                        row["name"], callee, helper))
                if not body_missing and (group := group_defect(
                        key, refs, body)):
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

    return checked, sorted(missing, key=order)


def render(row: MissingCall | MissingDefinition | MisgroupedCalls |
           ContractViolation | FileContractViolation) -> str:
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


def run_gate() -> list[str]:
    broken = selftest()
    if broken:
        return [f"dc-source-shape SELFTEST BROKEN: {item}" for item in broken]
    checked, missing = scan()
    if not missing:
        print(f"[build] dc-source-shape: {checked} DC source definitions; "
              "no missing ordinary helpers")
        return []
    shown = [render(row) for row in missing[:BUILD_REPORT_LIMIT]]
    remainder = len(missing) - len(shown)
    if remainder:
        shown.append(
            f"DC SOURCE SHAPE: {remainder} additional omission(s) hidden; "
            "run `python3 -m homm3.match.dc_source_shape` for the full list")
    return shown


def main(argv=None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if "--selftest" in argv:
        broken = selftest()
        for item in broken:
            print(f"SELFTEST BROKEN: {item}", file=sys.stderr)
        print("selftest OK" if not broken else "selftest FAILED")
        return 2 if broken else 0
    checked, missing = scan()
    for row in missing:
        print(render(row))
    print(f"checked {checked} DC source definitions; "
          f"{len(missing)} source-shape defect(s)")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
