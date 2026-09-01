#!/usr/bin/env python3
"""Read-only Dreamcast source-shape navigation (`homm3 dreamcast`).

The Dreamcast executable is an older RoE pressing built for WinCE/SH4.  Its
NB11 CodeView stream preserves names, source lines, lexical blocks and a
lower-bound local inventory that retail Complete's optimized, stripped x86
image does not.  This command joins those records into one function dossier.

Subcommands
-----------
  show SELECTOR [--json]
        Function identity, signature/retail bridges, parameters and locals,
        lexical scopes, and a statement-ordered call/branch listing.

        SELECTOR is one of:
          0x00403ee0                 retail VA (or 0x3ee0 retail RVA)
          dc:0x1190                  Dreamcast .text offset
          adventuremapwindow.obj:0x1190
          TAdventureMapWindow::ClearBottomView

  asm SELECTOR [--blocks] [--no-breakpoints] [--json]
        SH4 assembly labelled with CodeView breakpoint/source rows. --blocks
        adds inferred CFG predecessor/successor headers; lexical S_BLOCK32
        scopes remain separately labelled.

  find TEXT [--module MODULE] [--limit N] [--json]
        Search the complete Dreamcast procedure roster.  Exact names are not
        required; boundaries, module, source location and any retail bridge
        are shown.

  gaps [SELECTOR] [--minimum N|--exact N] [--retail-only] [--limit N] [--json]
        Reconstruct Vostok-style leading source-line gaps: the procedure-frame
        line is known, the first body line is known, and absent line numbers
        between them are zero-emission source candidates.  With no selector,
        rank the whole corpus by leading-gap size.

  stats [--json]
        Corpus coverage and bridge counts.

All results are ANALYSIS OUTPUT about another pressing.  A Dreamcast address,
line or call is never retail address/byte evidence; retail promotion still
requires corroboration from the pinned Complete executable.
"""
from __future__ import annotations

import argparse
import csv
import json
import re
import shlex
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, TextIO

from homm3.analysis import dc_asm, dc_lines, dc_srclines, debug_shape
from homm3.core import common


FUNCTIONS = common.EVIDENCE_DIR / "dreamcast/functions.csv"
VARIABLES = common.EVIDENCE_DIR / "dreamcast/variables.csv"
BRIDGES = common.EVIDENCE_DIR / "retail-dc-name-map.csv"
SRC_DIR = common.HOMM3_DIR / "src"
LOG = common.HOMM3_DIR / "build/homm3_dreamcast.log"

DC_EXE_SHA256 = \
    "cdbc7e75bd7d057171fa12b728aaaee01c1db133fff350b034950dd21dd07736"
DC_EXE_SIZE = 8425752
AUTHORITY = "Dreamcast RoE/WinCE reference; analysis output, not retail evidence"
GAP_CAUTION = (
    "A missing line-program row can be a blank, comment, declaration, brace, "
    "preprocessor-only line, or compiled-out statement (including an assert); "
    "it is a candidate, never assert proof."
)


class DreamcastError(ValueError):
    pass


@dataclass(frozen=True)
class Claim:
    va: int
    module: str
    dc_offset: int
    path: str
    line: int


@dataclass(frozen=True)
class DreamcastDossier:
    """Dreamcast-specific evidence wrapped around the neutral debug IR."""

    shape: debug_shape.DebugFunctionShape
    signatures: tuple[str, ...]
    retail_bridges: tuple[dict[str, Any], ...]
    retail_source_claims: tuple[dict[str, Any], ...]

    def to_dict(self) -> dict[str, Any]:
        return {
            "authority": AUTHORITY,
            "gap_caution": GAP_CAUTION,
            "debug_shape": self.shape.to_dict(),
            "signatures": list(self.signatures),
            "retail_bridges": list(self.retail_bridges),
            "retail_source_claims": list(self.retail_source_claims),
        }


def _csv_rows(path: Path) -> list[dict[str, str]]:
    if not path.is_file():
        raise DreamcastError(f"missing corpus file: {path}")
    with path.open(newline="") as fh:
        return list(csv.DictReader(line for line in fh
                                   if not line.startswith("#")))


def _integer(value: str | int) -> int:
    return value if isinstance(value, int) else int(value, 0)


def _basename(path: str) -> str:
    return path.rsplit("\\", 1)[-1].rsplit("/", 1)[-1]


def _source_claims(src_dir: Path = SRC_DIR) -> list[Claim]:
    claims: list[Claim] = []
    for path in sorted(src_dir.glob("*.cpp")):
        text = path.read_text(errors="replace")
        module = path.stem + ".obj"
        for match in dc_srclines.CLAIM_RE.finditer(text):
            claims.append(Claim(
                va=int(match.group(1), 16), module=module,
                dc_offset=int(match.group(2), 16),
                path=str(path.relative_to(common.HOMM3_DIR)),
                line=text.count("\n", 0, match.start()) + 1))
    return claims


class Corpus:
    """Materialized CSV indexes.  Optional rows make resolution hermetic in tests."""

    def __init__(self, *, functions: list[dict[str, str]] | None = None,
                 variables: list[dict[str, str]] | None = None,
                 bridges: list[dict[str, str]] | None = None,
                 claims: list[Claim] | None = None):
        self.functions = _csv_rows(FUNCTIONS) if functions is None else functions
        self.variables = _csv_rows(VARIABLES) if variables is None else variables
        self.bridges = _csv_rows(BRIDGES) if bridges is None else bridges
        self.claims = _source_claims() if claims is None else claims

        self.by_key: dict[tuple[str, int], dict[str, str]] = {}
        self.by_offset: dict[int, list[dict[str, str]]] = defaultdict(list)
        self.by_name: dict[str, list[dict[str, str]]] = defaultdict(list)
        for row in self.functions:
            key = (row["module"], _integer(row["offset"]))
            self.by_key[key] = row
            self.by_offset[key[1]].append(row)
            self.by_name[row["name"]].append(row)

        self.variables_by_key: dict[tuple[str, str], list[dict[str, str]]] = \
            defaultdict(list)
        for row in self.variables:
            self.variables_by_key[(row["module"], row["proc"])].append(row)

        self.bridges_by_key: dict[tuple[str, int], list[dict[str, str]]] = \
            defaultdict(list)
        self.bridges_by_rva: dict[int, list[dict[str, str]]] = defaultdict(list)
        for row in self.bridges:
            key = (row["dc_module"], _integer(row["dc_offset"]))
            self.bridges_by_key[key].append(row)
            self.bridges_by_rva[_integer(row["rva"])].append(row)

        self.claims_by_key: dict[tuple[str, int], list[Claim]] = defaultdict(list)
        self.claims_by_va: dict[int, list[Claim]] = defaultdict(list)
        for claim in self.claims:
            self.claims_by_key[(claim.module, claim.dc_offset)].append(claim)
            self.claims_by_va[claim.va].append(claim)

        # NB11's SRCLINES table is flat per compiland.  A previous procedure's
        # closing-brace row can therefore sit exactly at the next procedure's
        # address.  Keep source-local predecessor identity so the line-gap view
        # can reject that borrowed boundary instead of reporting a giant gap.
        self.previous_by_key: dict[tuple[str, int], dict[str, str]] = {}
        by_source: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)
        for function in self.functions:
            source = function["file"].replace("/", "\\").lower()
            by_source[(function["module"], source)].append(function)
        for functions in by_source.values():
            functions.sort(key=lambda item: _integer(item["offset"]))
            for previous, current in zip(functions, functions[1:]):
                self.previous_by_key[self.key(current)] = previous

    @staticmethod
    def key(row: dict[str, str]) -> tuple[str, int]:
        return row["module"], _integer(row["offset"])

    def _one(self, selector: str, rows: Iterable[dict[str, str]]) \
            -> dict[str, str]:
        unique = {self.key(row): row for row in rows}
        if not unique:
            raise DreamcastError(f"{selector}: no Dreamcast procedure matches")
        if len(unique) != 1:
            shown = sorted(unique.values(), key=lambda r: (r["module"],
                                                            _integer(r["offset"])))
            detail = ", ".join(
                f"{r['module']}:{r['offset']} {r['name']}" for r in shown[:8])
            more = f" (+{len(shown) - 8} more)" if len(shown) > 8 else ""
            raise DreamcastError(f"{selector}: ambiguous: {detail}{more}")
        return next(iter(unique.values()))

    def resolve(self, selector: str) -> dict[str, str]:
        """Resolve retail address, explicit DC address, module:offset or name."""
        if selector.lower().startswith("dc:"):
            try:
                off = _integer(selector.split(":", 1)[1])
            except ValueError as exc:
                raise DreamcastError(f"{selector}: invalid DC offset") from exc
            return self._one(selector, self.by_offset.get(off, ()))

        module_match = re.fullmatch(r"([^:]+\.obj):(0x[0-9a-fA-F]+)", selector)
        if module_match:
            key = (module_match.group(1), int(module_match.group(2), 16))
            row = self.by_key.get(key)
            if row is None:
                raise DreamcastError(f"{selector}: no Dreamcast roster row")
            return row

        if re.fullmatch(r"0x[0-9a-fA-F]+", selector):
            value = int(selector, 16)
            va = value if value >= common.IMAGE_BASE else value + common.IMAGE_BASE
            rva = va - common.IMAGE_BASE
            rows: list[dict[str, str]] = []
            for claim in self.claims_by_va.get(va, ()):
                row = self.by_key.get((claim.module, claim.dc_offset))
                if row is not None:
                    rows.append(row)
            for bridge in self.bridges_by_rva.get(rva, ()):
                row = self.by_key.get((bridge["dc_module"],
                                       _integer(bridge["dc_offset"])))
                if row is not None:
                    rows.append(row)
            if not rows:
                raise DreamcastError(
                    f"{selector}: no source claim or retail/DC bridge; use "
                    "dc:0xOFF for a Dreamcast offset")
            return self._one(selector, rows)

        exact = self.by_name.get(selector, ())
        if exact:
            return self._one(selector, exact)
        folded = [row for name, rows in self.by_name.items()
                  if name.lower() == selector.lower() for row in rows]
        if folded:
            return self._one(selector, folded)
        needle = selector.lower()
        return self._one(selector, (row for row in self.functions
                                    if needle in row["name"].lower()))

    def find(self, pattern: str, module: str | None = None) \
            -> list[dict[str, str]]:
        needle = pattern.lower()
        if module and not module.endswith(".obj"):
            module += ".obj"
        rows = [row for row in self.functions
                if needle in row["name"].lower()
                and (module is None or row["module"] == module)]
        return sorted(rows, key=lambda r: (r["module"], _integer(r["offset"]),
                                           r["name"]))


def _gate_dc_exe(path: Path) -> bytes:
    if not path.is_file():
        raise DreamcastError(f"Dreamcast executable not found: {path}")
    size = path.stat().st_size
    if size != DC_EXE_SIZE:
        raise DreamcastError(
            f"{path}: size {size} != pinned Dreamcast size {DC_EXE_SIZE}")
    digest = common.sha256_of(path)
    if digest != DC_EXE_SHA256:
        raise DreamcastError(
            f"{path}: sha256 {digest} != pinned Dreamcast {DC_EXE_SHA256}")
    return path.read_bytes()


def _top_level_scopes(name: str) -> list[str]:
    parts: list[str] = []
    depth = start = index = 0
    while index < len(name):
        char = name[index]
        if char == "<":
            depth += 1
        elif char == ">" and depth:
            depth -= 1
        elif char == ":" and index + 1 < len(name) \
                and name[index + 1] == ":" and depth == 0:
            parts.append(name[start:index])
            start = index + 2
            index += 1
        index += 1
    parts.append(name[start:])
    return parts


def _call_kind(name: str | None, size: int | None) -> str | None:
    if not name:
        return None
    if name.startswith("??0"):
        return "constructor"
    if name.startswith(("??1", "??_E", "??_G")):
        return "destructor"
    scopes = _top_level_scopes(name)
    if scopes[-1].startswith("~"):
        return "destructor"
    if len(scopes) >= 2:
        owner = scopes[-2].split("<", 1)[0]
        method = scopes[-1].split("<", 1)[0]
        if method == owner:
            return "constructor"
    if size is not None and size <= 16:
        return "tiny-helper"
    return "call"


def _retail_bridges(corpus: Corpus, key: tuple[str, int]) -> list[dict[str, Any]]:
    out = []
    for row in corpus.bridges_by_key.get(key, ()):
        rva = _integer(row["rva"])
        out.append({
            "va": common.IMAGE_BASE + rva,
            "rva": rva,
            "size": _integer(row["size"]),
            "role": row["role"],
            "name": row["name"],
            "signature": row["signature"],
            "source": row["source"],
        })
    return out


def build_dossier(corpus: Corpus, row: dict[str, str]) -> DreamcastDossier:
    if not dc_lines.DUMP.is_file():
        raise DreamcastError(f"Dreamcast CodeView dump not found: {dc_lines.DUMP}")
    off, cb = _integer(row["offset"]), _integer(row["cb"])
    key = corpus.key(row)
    dump = dc_lines._dump_lines()
    proc = dc_lines.find_proc(dump, off)
    if proc is None:
        raise DreamcastError(f"dc {off:#x}: no S_GPROC32/S_LPROC32 record")
    proc_name, proc_cb, _raw_locals, blocks = proc
    if proc_cb != cb or proc_name != row["name"]:
        raise DreamcastError(
            f"dc {off:#x}: CSV/dump disagreement ({row['name']} {cb} B vs "
            f"{proc_name} {proc_cb} B)")

    data = _gate_dc_exe(dc_lines.EXE)
    symbols = dc_lines.symbol_map(dump)
    statements = dc_lines.line_table(dump, off, cb)
    line_shape = _source_line_shape(
        row, previous_row=corpus.previous_by_key.get(key))
    bounds = [addr for addr, _line, _file in statements] + [off + cb]
    try:
        asm_view = dc_asm.build_view(row, dump, data)
    except dc_asm.AsmError as exc:
        raise DreamcastError(str(exc)) from exc
    control = dc_asm.control_events(asm_view, data)

    statement_rows: list[debug_shape.DebugStatement] = []
    for index, (addr, line, source) in enumerate(statements):
        end = bounds[index + 1]
        statement_events = [(site, event)
                            for site, event in sorted(control.items())
                            if addr <= site < end]
        calls = [(site, event["call_target_va"])
                 for site, event in statement_events
                 if "call_target_va" in event]
        branches = sum(event.get("conditional_branch", False)
                       for _site, event in statement_events)
        rendered_calls: list[debug_shape.DebugCall] = []
        for site, target in calls:
            name = symbols.get(target) if target is not None else None
            dc_target = None
            target_row = None
            if target is not None and dc_lines.POOL_BASE <= target:
                candidate = target - dc_lines.POOL_BASE
                hits = corpus.by_offset.get(candidate, ())
                if len(hits) == 1:
                    dc_target, target_row = candidate, hits[0]
                    # S_GPROC32 carries the human procedure name while a
                    # colliding S_PUB32 record may carry only its decoration.
                    # Prefer the typed roster identity for helper/RAII reads.
                    name = target_row["name"]
            target_size = _integer(target_row["cb"]) if target_row else None
            rendered_calls.append(debug_shape.DebugCall(
                site_address=site,
                target_address=target,
                target_function_address=dc_target,
                target_emitted_size=target_size,
                name=name,
                classification=_call_kind(name, target_size),
            ))
        statement_rows.append(debug_shape.DebugStatement(
            address=addr,
            emitted_size=end - addr,
            source_file=source,
            source_line=line,
            branch_count=branches,
            scope_opens=sum(1 for start, _size in blocks if start == addr),
            scope_closes=sum(1 for start, size in blocks
                             if start + size == addr),
            calls=tuple(rendered_calls),
        ))

    variables = corpus.variables_by_key.get((row["module"], row["name"]), ())
    def variable(item: dict[str, str]) -> debug_shape.DebugVariable:
        return debug_shape.DebugVariable(
            name=item["name"], type_name=item["type"],
            storage=item.get("sp_offset") or None)

    line_map = debug_shape.DebugLineMap(
        procedure_line=line_shape["procedure_line"],
        procedure_line_reliable=line_shape["procedure_line_reliable"],
        bodyless=line_shape["bodyless"],
        first_body_line=line_shape["first_body_line"],
        first_body_address=line_shape["first_body_address"],
        previous_body_last_line=line_shape["previous_body_last_line"],
        borrowed_boundary_line=line_shape["borrowed_boundary_line"],
        gaps=tuple(debug_shape.DebugSourceGap(
            after_line=item["after_line"],
            before_line=item["before_line"],
            first_missing_line=item["first_missing_line"],
            last_missing_line=item["last_missing_line"],
            leading=item["leading"],
        ) for item in line_shape["gaps"]),
    )

    shape = debug_shape.DebugFunctionShape(
        producer="CodeView NB11",
        target="Dreamcast WinCE SH4",
        address_space="Dreamcast .text offset",
        name=row["name"],
        module=row["module"],
        linkage=row["kind"],
        address=off,
        emitted_size=cb,
        source_file=row["file"],
        boundary_line=_integer(row["line"]),
        debug_start=_integer(row["debug_start"]),
        debug_end=_integer(row["debug_end"]),
        line_map=line_map,
        parameters=tuple(variable(item) for item in variables
                         if item["kind"] == "param"),
        locals=tuple(variable(item) for item in variables
                     if item["kind"] != "param"),
        scopes=debug_shape.scope_ranges(blocks),
        statements=tuple(statement_rows),
    )
    bridges = tuple(_retail_bridges(corpus, key))
    claims = tuple({"va": claim.va, "path": claim.path, "line": claim.line}
                   for claim in corpus.claims_by_key.get(key, ()))
    return DreamcastDossier(
        shape=shape,
        signatures=tuple(sorted({bridge["signature"] for bridge in bridges
                                 if bridge["signature"]})),
        retail_bridges=bridges,
        retail_source_claims=claims,
    )


def _hex(value: int | None, width: int = 0) -> str:
    if value is None:
        return "<indirect>"
    return f"0x{value:0{width}x}" if width else f"{value:#x}"


def render_dossier(dossier: DreamcastDossier,
                   out: TextIO = sys.stdout) -> None:
    shape = dossier.shape
    print("DREAMCAST REFERENCE — ANALYSIS OUTPUT, NOT RETAIL EVIDENCE", file=out)
    print(file=out)
    print(shape.name, file=out)
    print(f"  module       {shape.module} ({shape.linkage})", file=out)
    print(f"  source       {shape.source_file}:{shape.boundary_line} (boundary row)",
          file=out)
    print(f"  dc text      {_hex(shape.address, 8)}..{_hex(shape.end_address, 8)} "
          f"({shape.emitted_size} B SH4)", file=out)
    print(f"  debug body   +{_hex(shape.debug_start)}..+{_hex(shape.debug_end)}",
          file=out)
    for signature in dossier.signatures:
        print(f"  signature    {signature}", file=out)

    print("\nRetail bridge(s) — correlation only:", file=out)
    if not dossier.retail_bridges and not dossier.retail_source_claims:
        print("  none", file=out)
    for bridge in dossier.retail_bridges:
        print(f"  VA {_hex(bridge['va'], 8)}  RVA {_hex(bridge['rva'], 8)}  "
              f"{bridge['size']} B  role={bridge['role']}", file=out)
    for claim in dossier.retail_source_claims:
        print(f"  source claim {_hex(claim['va'], 8)}  "
              f"{claim['path']}:{claim['line']}", file=out)

    for title, rows in (("Parameters", shape.parameters),
                        ("Locals", shape.locals)):
        print(f"\n{title} ({len(rows)}; CodeView lower bound):", file=out)
        if not rows:
            print("  none recorded", file=out)
        for item in rows:
            storage = item.storage or "-"
            print(f"  {storage:>9}  {item.type_name:<20}  {item.name}",
                  file=out)

    print(f"\nLexical scopes ({len(shape.scopes)}):", file=out)
    if not shape.scopes:
        print("  none recorded", file=out)
    for scope in shape.scopes:
        print(f"  {_hex(scope.start, 8)}..{_hex(scope.end, 8)}  "
              f"{scope.emitted_size} B  depth={scope.depth}", file=out)

    line_map = shape.line_map
    assert line_map is not None
    print("\nSource-line gaps (zero-emission candidates):", file=out)
    if line_map.bodyless:
        print("  minimal 4-byte SH4 body; no body-line inference", file=out)
    elif not line_map.procedure_line_reliable:
        print(f"  unavailable: boundary line {shape.boundary_line} is coherent "
              "with the preceding procedure's closing row", file=out)
    elif line_map.first_body_line is None:
        print("  first body line unavailable", file=out)
    else:
        missing = line_map.leading_gap_lines
        assert line_map.procedure_line is not None
        span = _line_span(line_map.procedure_line + 1,
                          line_map.first_body_line - 1)
        detail = f"; absent {span}" if missing else ""
        print(f"  leading: frame line {line_map.procedure_line} -> first body line "
              f"{line_map.first_body_line}: {missing} absent line(s){detail}",
              file=out)
    print(f"  {GAP_CAUTION}", file=out)

    print(f"\nStatements ({len(shape.statements)} line/address rows):", file=out)
    for statement in shape.statements:
        marks = " {" * statement.scope_opens + " }" * statement.scope_closes
        branch = f" br={statement.branch_count}" \
            if statement.branch_count else ""
        print(f"  {_basename(statement.source_file)}:{statement.source_line:<5} "
              f"dc {_hex(statement.address, 8)} {statement.emitted_size:>4} B"
              f"{branch}{marks}", file=out)
        for call in statement.calls:
            name = call.name or _hex(call.target_address)
            details = []
            if call.classification and call.classification != "call":
                details.append(call.classification)
            if call.target_function_address is not None:
                details.append(f"dc {_hex(call.target_function_address)}")
            if call.target_emitted_size is not None:
                details.append(f"{call.target_emitted_size} B")
            suffix = f"  [{', '.join(details)}]" if details else ""
            print(f"      -> {name}{suffix}", file=out)

    summary = shape.summary()
    print("\nSummary:", file=out)
    print(f"  source rows={summary['statement_rows']}  "
          f"branches={summary['conditional_branches']}  calls={summary['calls']}  "
          f"unique callees={summary['unique_named_callees']}", file=out)
    print(f"  ctors={summary['constructor_calls']}  dtors={summary['destructor_calls']}  "
          f"tiny-helper calls={summary['tiny_helper_calls']}", file=out)
    print("\nCaution: DC source is older and platform-specific. Promote a source fact "
          "only after retail x86 corroboration.", file=out)


def _mapped_vas(corpus: Corpus, row: dict[str, str]) -> list[int]:
    key = corpus.key(row)
    values = {common.IMAGE_BASE + _integer(item["rva"])
              for item in corpus.bridges_by_key.get(key, ())}
    values.update(claim.va for claim in corpus.claims_by_key.get(key, ()))
    return sorted(values)


def _source_key(path: str) -> str:
    return path.replace("/", "\\").lower()


def _line_span(first: int, last: int) -> str:
    if first == last:
        return str(first)
    return f"{first}..{last}"


def _source_line_shape(
        row: dict[str, str],
        module_rows: Iterable[tuple[str, int, int]] | None = None,
        previous_row: dict[str, str] | None = None,
) -> dict[str, Any]:
    """Recover source-line holes using the same frame/body idea as Vostok.

    The first CodeView line at the procedure boundary is the procedure-frame
    row.  The next lexical source line represented inside the procedure is the
    first body row.  Missing integers between the two are source lines for which
    the optimized compiland emitted no line-program row.

    This is deliberately source-only evidence.  It cannot distinguish an empty
    line, comment, declaration, brace, preprocessor line, or folded statement.
    A four-byte SH4 body is just ``rts; nop`` and is treated as body-less rather
    than turning its closing-frame line into a giant false leading gap.
    """
    off, cb = _integer(row["offset"]), _integer(row["cb"])
    boundary_line = _integer(row["line"]) if row.get("line") else None
    if module_rows is None:
        module_rows = dc_srclines._load_srclines().get(row["module"], ())
    source = _source_key(row["file"])
    records = [(line, addr) for filename, line, addr in module_rows
               if source == _source_key(filename) and off <= addr < off + cb]
    records.sort(key=lambda item: item[1])

    # Preserve distinct (line,address) rows: two rows on one source line can be
    # the frame and a one-line body, while exact duplicate dump rows add nothing.
    seen: set[tuple[int, int]] = set()
    unique_records = []
    for item in records:
        if item not in seen:
            seen.add(item)
            unique_records.append(item)
    records = unique_records
    if boundary_line is None and records:
        boundary_line = records[0][0]

    line_counts = Counter(line for line, _addr in records)
    lexical_lines = sorted({line for line, _addr in records
                            if boundary_line is None or line >= boundary_line})
    bodyless = cb == 4
    first_body_line: int | None = None
    first_body_address: int | None = None
    if not bodyless and boundary_line is not None:
        if line_counts[boundary_line] > 1:
            first_body_line = boundary_line
        else:
            first_body_line = next(
                (line for line in lexical_lines if line > boundary_line), None)
        if first_body_line is not None:
            first_body_address = next(
                (addr for line, addr in records if line == first_body_line), None)

    previous_last_line = None
    boundary_borrowed = False
    previous_end = None
    if previous_row is not None:
        previous_end = (_integer(previous_row["offset"])
                        + _integer(previous_row["cb"]))
    if (previous_row is not None and boundary_line is not None
            and first_body_line is not None and first_body_line > boundary_line
            and line_counts[boundary_line] == 1
            # SH4 procedures are four-byte aligned, so the preceding closing
            # row can land after its two-byte pad at the next aligned entry.
            and previous_end is not None and 0 <= off - previous_end <= 2):
        previous_off = _integer(previous_row["offset"])
        previous_lines = [line for filename, line, addr in module_rows
                          if source == _source_key(filename)
                          and previous_off <= addr < previous_end]
        if previous_lines:
            previous_last_line = max(previous_lines)
            distance_to_previous = abs(boundary_line - previous_last_line)
            distance_to_body = first_body_line - boundary_line
            boundary_borrowed = distance_to_previous < distance_to_body

    procedure_line = None if boundary_borrowed else boundary_line
    if boundary_borrowed:
        lexical_lines = [line for line in lexical_lines if line > boundary_line]

    gaps = []
    for before, after in zip(lexical_lines, lexical_lines[1:]):
        missing = after - before - 1
        if missing > 0:
            gaps.append({
                "after_line": before,
                "before_line": after,
                "missing_lines": missing,
                "first_missing_line": before + 1,
                "last_missing_line": after - 1,
                "leading": procedure_line is not None and before == procedure_line,
            })

    leading_gap = None
    if procedure_line is not None and first_body_line is not None:
        leading_gap = max(0, first_body_line - procedure_line - 1)
    return {
        "source": row["file"],
        "boundary_line": boundary_line,
        "procedure_line": procedure_line,
        "procedure_line_reliable": procedure_line is not None,
        "borrowed_boundary_line": boundary_borrowed,
        "previous_body_last_line": previous_last_line,
        "procedure_address": off,
        "first_body_line": first_body_line,
        "first_body_address": first_body_address,
        "leading_gap_lines": leading_gap,
        "bodyless": bodyless,
        "gaps": gaps,
        "caution": GAP_CAUTION,
    }


def _gap_payload(corpus: Corpus, row: dict[str, str]) -> dict[str, Any]:
    shape = _source_line_shape(
        row, previous_row=corpus.previous_by_key.get(corpus.key(row)))
    return {
        "name": row["name"],
        "module": row["module"],
        "dc_offset": _integer(row["offset"]),
        "dc_size": _integer(row["cb"]),
        "retail_vas": _mapped_vas(corpus, row),
        **shape,
    }


def _render_gaps(rows: list[dict[str, Any]], *, selected: bool,
                 out: TextIO = sys.stdout) -> None:
    print("DREAMCAST LEADING SOURCE-LINE GAPS — ANALYSIS OUTPUT, NOT RETAIL EVIDENCE",
          file=out)
    print(GAP_CAUTION, file=out)
    if not rows:
        print("no qualifying leading gaps", file=out)
        return
    for row in rows:
        gap = row["leading_gap_lines"]
        gap_text = "unavailable" if gap is None else str(gap)
        retail = ",".join(_hex(va, 8) for va in row["retail_vas"]) or "-"
        print(f"\n{gap_text:>11} absent  {row['module']}:{_hex(row['dc_offset'])}  "
              f"retail={retail}", file=out)
        print(f"  {row['name']}", file=out)
        if row["bodyless"]:
            print(f"  {_basename(row['source'])}:{row['boundary_line']}  "
                  "minimal 4-byte SH4 body", file=out)
        elif not row["procedure_line_reliable"]:
            print(f"  {_basename(row['source'])}: boundary {row['boundary_line']} "
                  f"borrowed from preceding body ending near "
                  f"{row['previous_body_last_line']}; leading gap unavailable",
                  file=out)
        elif row["first_body_line"] is None:
            print(f"  {_basename(row['source'])}:{row['boundary_line']}  "
                  "first body line unavailable", file=out)
        else:
            first = row["procedure_line"] + 1
            last = row["first_body_line"] - 1
            span = f"; absent {_line_span(first, last)}" if gap else ""
            print(f"  {_basename(row['source'])}: frame {row['procedure_line']} -> "
                  f"body {row['first_body_line']}{span}", file=out)
        if selected and row["gaps"]:
            print("  all source-line gaps:", file=out)
            for item in row["gaps"]:
                tag = " leading" if item["leading"] else ""
                span = _line_span(item["first_missing_line"],
                                  item["last_missing_line"])
                print(f"    {span} ({item['missing_lines']} line(s)){tag}", file=out)


def _find_payload(corpus: Corpus, rows: list[dict[str, str]]) -> list[dict[str, Any]]:
    return [{
        "name": row["name"], "module": row["module"],
        "dc_offset": _integer(row["offset"]), "dc_size": _integer(row["cb"]),
        "source": row["file"], "line": _integer(row["line"]),
        "retail_vas": _mapped_vas(corpus, row),
    } for row in rows]


def _render_find(rows: list[dict[str, Any]], out: TextIO = sys.stdout) -> None:
    print("Dreamcast CodeView corpus — analysis output, not retail evidence",
          file=out)
    if not rows:
        print("no Dreamcast procedure names matched", file=out)
        return
    for row in rows:
        retail = ",".join(_hex(va, 8) for va in row["retail_vas"]) or "-"
        print(f"{_hex(row['dc_offset'], 8)} {row['dc_size']:>6} B  "
              f"{row['module']:<30} retail={retail}", file=out)
        print(f"  {row['name']}  ({_basename(row['source'])}:{row['line']})", file=out)


def _stats(corpus: Corpus) -> dict[str, Any]:
    sizes = [_integer(row["cb"]) for row in corpus.functions]
    keys_with_bridge = set(corpus.bridges_by_key)
    keys_with_claim = set(corpus.claims_by_key)
    return {
        "authority": AUTHORITY,
        "functions": len(corpus.functions),
        "modules": len({row["module"] for row in corpus.functions}),
        "parameters": sum(row["kind"] == "param" for row in corpus.variables),
        "locals": sum(row["kind"] != "param" for row in corpus.variables),
        "functions_with_recorded_locals": len({(row["module"], row["proc"])
                                               for row in corpus.variables
                                               if row["kind"] != "param"}),
        "retail_bridge_rows": len(corpus.bridges),
        "functions_with_retail_bridge": len(keys_with_bridge),
        "functions_with_source_claim": len(keys_with_claim),
        "four_byte_functions": sum(size == 4 for size in sizes),
        "bridge_roles": dict(sorted(Counter(row["role"]
                                            for row in corpus.bridges).items())),
    }


def _render_stats(stats: dict[str, Any], out: TextIO = sys.stdout) -> None:
    print(stats["authority"], file=out)
    print(f"  functions                    {stats['functions']}", file=out)
    print(f"  modules                      {stats['modules']}", file=out)
    print(f"  parameters                   {stats['parameters']}", file=out)
    print(f"  locals                       {stats['locals']}", file=out)
    print(f"  functions with locals        {stats['functions_with_recorded_locals']}",
          file=out)
    print(f"  four-byte functions          {stats['four_byte_functions']}", file=out)
    print(f"  retail bridge rows           {stats['retail_bridge_rows']}", file=out)
    print(f"  functions with bridge        {stats['functions_with_retail_bridge']}",
          file=out)
    print(f"  functions with source claim  {stats['functions_with_source_claim']}",
          file=out)
    print("  bridge roles                 " + ", ".join(
        f"{name}={count}" for name, count in stats["bridge_roles"].items()), file=out)


def _build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(
        prog="homm3 dreamcast", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="command", required=True)

    show = sub.add_parser("show", help="joined source-shape dossier")
    show.add_argument("selector", help="retail VA/RVA, dc:OFF, module:OFF, or name")
    show.add_argument("--json", action="store_true", help="machine-readable output")

    asm = sub.add_parser("asm", help="breakpoint-labelled SH4 assembly / CFG")
    asm.add_argument("selector", help="retail VA/RVA, dc:OFF, module:OFF, or name")
    asm.add_argument("--blocks", action="store_true",
                     help="show inferred CFG predecessor/successor headers")
    asm.add_argument("--no-breakpoints", action="store_true",
                     help="suppress CodeView source-line labels")
    asm.add_argument("--json", action="store_true", help="machine-readable output")

    find = sub.add_parser("find", help="search Dreamcast procedure names")
    find.add_argument("text", help="case-insensitive name substring")
    find.add_argument("--module", help="restrict to one module[.obj]")
    find.add_argument("--limit", type=int, default=50,
                      help="maximum rows (default 50; 0 = all)")
    find.add_argument("--json", action="store_true", help="machine-readable output")

    gaps = sub.add_parser("gaps", help="Vostok-style leading source-line gaps")
    gaps.add_argument("selector", nargs="?",
                      help="optional retail VA/RVA, dc:OFF, module:OFF, or name")
    gap_filter = gaps.add_mutually_exclusive_group()
    gap_filter.add_argument(
        "--minimum", type=int, default=1,
        help="minimum absent leading lines in corpus mode (default 1)")
    gap_filter.add_argument(
        "--exact", type=int,
        help="require exactly N absent leading lines in corpus mode")
    gaps.add_argument("--retail-only", action="store_true",
                      help="only procedures carrying a retail bridge or claim")
    gaps.add_argument("--limit", type=int, default=50,
                      help="maximum corpus rows (default 50; 0 = all)")
    gaps.add_argument("--json", action="store_true", help="machine-readable output")

    stats = sub.add_parser("stats", help="corpus and retail-bridge coverage")
    stats.add_argument("--json", action="store_true", help="machine-readable output")
    return ap


def _log(rc: int, argv: list[str]) -> None:
    try:
        import datetime
        now = datetime.datetime.now()
        LOG.parent.mkdir(parents=True, exist_ok=True)
        with LOG.open("a") as fh:
            fh.write(f"[{now.date()}][{now.strftime('%H:%M:%S')}][{rc}]: "
                     f"{shlex.join(['homm3', 'dreamcast', *argv])}\n")
    except Exception:
        pass


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    parser = _build_parser()
    rc = 0
    try:
        args = parser.parse_args(argv)
        corpus = Corpus()
        if args.command == "show":
            row = corpus.resolve(args.selector)
            dossier = build_dossier(corpus, row)
            if args.json:
                json.dump(dossier.to_dict(), sys.stdout,
                          indent=2, sort_keys=True)
                print()
            else:
                render_dossier(dossier)
        elif args.command == "asm":
            row = corpus.resolve(args.selector)
            if not dc_lines.DUMP.is_file():
                raise DreamcastError(
                    f"Dreamcast CodeView dump not found: {dc_lines.DUMP}")
            dump = dc_lines._dump_lines()
            data = _gate_dc_exe(dc_lines.EXE)
            try:
                view = dc_asm.build_view(row, dump, data)
            except dc_asm.AsmError as exc:
                raise DreamcastError(str(exc)) from exc
            if args.json:
                json.dump(view, sys.stdout, indent=2, sort_keys=True)
                print()
            else:
                dc_asm.render(view, data, dc_lines.symbol_map(dump),
                              blocks=args.blocks,
                              breakpoints=not args.no_breakpoints,
                              out=sys.stdout)
        elif args.command == "find":
            rows = corpus.find(args.text, args.module)
            if args.limit < 0:
                raise DreamcastError("--limit must be >= 0")
            if args.limit:
                rows = rows[:args.limit]
            payload = _find_payload(corpus, rows)
            if args.json:
                json.dump({"authority": AUTHORITY, "matches": payload},
                          sys.stdout, indent=2, sort_keys=True)
                print()
            else:
                _render_find(payload)
        elif args.command == "gaps":
            if args.minimum < 0:
                raise DreamcastError("--minimum must be >= 0")
            if args.exact is not None and args.exact < 0:
                raise DreamcastError("--exact must be >= 0")
            if args.limit < 0:
                raise DreamcastError("--limit must be >= 0")
            if args.selector:
                rows = [_gap_payload(corpus, corpus.resolve(args.selector))]
            else:
                rows = [_gap_payload(corpus, row) for row in corpus.functions]
                rows = [row for row in rows
                        if row["leading_gap_lines"] is not None
                        and ((args.exact is not None
                              and row["leading_gap_lines"] == args.exact)
                             or (args.exact is None
                                 and row["leading_gap_lines"] >= args.minimum))
                        and (not args.retail_only or row["retail_vas"])]
                rows.sort(key=lambda row: (-row["leading_gap_lines"],
                                           row["module"], row["dc_offset"]))
                if args.limit:
                    rows = rows[:args.limit]
            payload = {
                "authority": AUTHORITY,
                "caution": GAP_CAUTION,
                "selected": bool(args.selector),
                "candidates": rows,
            }
            if args.json:
                json.dump(payload, sys.stdout, indent=2, sort_keys=True)
                print()
            else:
                _render_gaps(rows, selected=bool(args.selector))
        else:
            payload = _stats(corpus)
            if args.json:
                json.dump(payload, sys.stdout, indent=2, sort_keys=True)
                print()
            else:
                _render_stats(payload)
    except DreamcastError as exc:
        print(f"[homm3 dreamcast] ERROR: {exc}", file=sys.stderr)
        rc = 2
    except SystemExit as exc:
        rc = exc.code if isinstance(exc.code, int) else 2
        _log(rc, argv)
        raise
    _log(rc, argv)
    return rc


if __name__ == "__main__":
    sys.exit(main())
