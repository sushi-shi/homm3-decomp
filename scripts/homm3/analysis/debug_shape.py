"""Compiler-neutral source/debug structure recovered from symbol records.

Parsers for CodeView, PDBs, DWARF, or another debug format may all populate
this model.  It describes what a debugger knows about one compiled function;
it deliberately contains no comparison policy and no candidate/retail link.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


SCHEMA = "homm3.debug-shape.v1"


@dataclass(frozen=True)
class DebugVariable:
    name: str
    type_name: str
    storage: str | None = None

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "type": self.type_name,
            "storage": self.storage,
        }


@dataclass(frozen=True)
class DebugSourceGap:
    after_line: int
    before_line: int
    first_missing_line: int
    last_missing_line: int
    leading: bool = False

    @property
    def missing_lines(self) -> int:
        return self.last_missing_line - self.first_missing_line + 1

    def to_dict(self) -> dict[str, Any]:
        return {
            "after_line": self.after_line,
            "before_line": self.before_line,
            "first_missing_line": self.first_missing_line,
            "last_missing_line": self.last_missing_line,
            "missing_lines": self.missing_lines,
            "leading": self.leading,
        }


@dataclass(frozen=True)
class DebugLineMap:
    procedure_line: int | None
    procedure_line_reliable: bool
    bodyless: bool
    first_body_line: int | None = None
    first_body_address: int | None = None
    previous_body_last_line: int | None = None
    borrowed_boundary_line: bool = False
    gaps: tuple[DebugSourceGap, ...] = field(default_factory=tuple)

    @property
    def leading_gap_lines(self) -> int | None:
        if self.bodyless or self.procedure_line is None \
                or not self.procedure_line_reliable \
                or self.first_body_line is None:
            return None
        return max(0, self.first_body_line - self.procedure_line - 1)

    def to_dict(self) -> dict[str, Any]:
        return {
            "procedure_line": self.procedure_line,
            "procedure_line_reliable": self.procedure_line_reliable,
            "bodyless": self.bodyless,
            "first_body_line": self.first_body_line,
            "first_body_address": self.first_body_address,
            "previous_body_last_line": self.previous_body_last_line,
            "borrowed_boundary_line": self.borrowed_boundary_line,
            "leading_gap_lines": self.leading_gap_lines,
            "gaps": [gap.to_dict() for gap in self.gaps],
        }


@dataclass(frozen=True)
class DebugCall:
    site_address: int
    target_address: int | None = None
    target_function_address: int | None = None
    target_emitted_size: int | None = None
    name: str | None = None
    classification: str | None = None

    def to_dict(self) -> dict[str, Any]:
        return {
            "site_address": self.site_address,
            "target_address": self.target_address,
            "target_function_address": self.target_function_address,
            "target_emitted_size": self.target_emitted_size,
            "name": self.name,
            "classification": self.classification,
        }


@dataclass(frozen=True)
class DebugStatement:
    address: int
    emitted_size: int
    source_file: str
    source_line: int
    branch_count: int = 0
    scope_opens: int = 0
    scope_closes: int = 0
    calls: tuple[DebugCall, ...] = field(default_factory=tuple)

    def to_dict(self) -> dict[str, Any]:
        return {
            "address": self.address,
            "emitted_size": self.emitted_size,
            "source": {"file": self.source_file, "line": self.source_line},
            "branch_count": self.branch_count,
            "scope_opens": self.scope_opens,
            "scope_closes": self.scope_closes,
            "calls": [call.to_dict() for call in self.calls],
        }


@dataclass(frozen=True)
class DebugScope:
    start: int
    end: int
    depth: int = 0

    @property
    def emitted_size(self) -> int:
        return self.end - self.start

    def to_dict(self) -> dict[str, Any]:
        return {
            "start": self.start,
            "end": self.end,
            "emitted_size": self.emitted_size,
            "depth": self.depth,
        }


@dataclass(frozen=True)
class DebugFunctionShape:
    producer: str
    target: str
    address_space: str
    name: str
    module: str
    linkage: str
    address: int
    emitted_size: int
    source_file: str
    boundary_line: int
    debug_start: int
    debug_end: int
    line_map: DebugLineMap | None = None
    parameters: tuple[DebugVariable, ...] = field(default_factory=tuple)
    locals: tuple[DebugVariable, ...] = field(default_factory=tuple)
    scopes: tuple[DebugScope, ...] = field(default_factory=tuple)
    statements: tuple[DebugStatement, ...] = field(default_factory=tuple)

    @property
    def end_address(self) -> int:
        return self.address + self.emitted_size

    def summary(self) -> dict[str, int]:
        calls = [call for statement in self.statements
                 for call in statement.calls]
        return {
            "statement_rows": len(self.statements),
            "distinct_source_lines": len({
                (statement.source_file, statement.source_line)
                for statement in self.statements
            }),
            "conditional_branches": sum(
                statement.branch_count for statement in self.statements),
            "calls": len(calls),
            "unique_named_callees": len({
                call.name for call in calls if call.name
            }),
            "constructor_calls": sum(
                call.classification == "constructor" for call in calls),
            "destructor_calls": sum(
                call.classification == "destructor" for call in calls),
            "tiny_helper_calls": sum(
                call.classification == "tiny-helper" for call in calls),
        }

    def to_dict(self) -> dict[str, Any]:
        return {
            "schema": SCHEMA,
            "producer": self.producer,
            "target": self.target,
            "address_space": self.address_space,
            "function": {
                "name": self.name,
                "module": self.module,
                "linkage": self.linkage,
                "address": self.address,
                "emitted_size": self.emitted_size,
                "end_address": self.end_address,
                "source": {
                    "file": self.source_file,
                    "boundary_line": self.boundary_line,
                },
                "debug_body": {
                    "start_offset": self.debug_start,
                    "end_offset": self.debug_end,
                },
            },
            "parameters": [item.to_dict() for item in self.parameters],
            "locals": [item.to_dict() for item in self.locals],
            "line_map": self.line_map.to_dict() if self.line_map else None,
            "scopes": [scope.to_dict() for scope in self.scopes],
            "statements": [statement.to_dict()
                           for statement in self.statements],
            "summary": self.summary(),
        }


def scope_ranges(rows: list[tuple[int, int]]) -> tuple[DebugScope, ...]:
    """Convert `(start, size)` records and derive lexical nesting depth."""
    ranges = [(start, start + size) for start, size in rows]
    return tuple(DebugScope(start, end, sum(
        outer_start <= start and end <= outer_end
        and (outer_start, outer_end) != (start, end)
        for outer_start, outer_end in ranges))
        for start, end in ranges)
